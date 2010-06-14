#include <iostream> // For std::streamsize

#include "block.h"
#include "coders.h"
#include "globaldefs.h"
#include "probmodels/base_prob_model.h"
#include "utils.h"

namespace bwtc {

uint64 PackInteger(uint64 integer, int* bytes_needed) {
  /* Results for the if-clause are undefined if integer value 0x80
   * doesn't have correct type (64-bit) */
  static const uint64 kEightBit = 0x80;

  uint64 result = 0; int i;
  // For optimization (if needed) two OR-operations could be merged
  for(i = 0; integer; ++i) {
    result |= ((integer & 0x7F) << i*8);
    integer >>= 7;
    assert(i < 8);
    if (integer) result |= (kEightBit << i*8); 
  }
  *bytes_needed = i;
  return result;
}

uint64 UnpackInteger(uint64 packed_integer) {
  uint64 result = 0; int bits_handled = 0;
  bool bits_left;
  do {
    bits_left = (packed_integer & 0x80) != 0;
    result |= ((packed_integer & 0x7F) << bits_handled);
    packed_integer >>= 8;
    bits_handled += 7;
    assert(bits_handled <= 56);
  } while(bits_left);
  return result;
}


ProbabilityModel* GiveProbabilityModel(char choice) {
  switch(choice) {
    case 'n':
    default:
        return new ProbabilityModel();
  }
}

Encoder::Encoder(const std::string& destination, char prob_model)
    : out_(NULL), destination_(NULL), pm_(NULL) {
  out_ = new OutStream(destination);
  destination_ = new dcsbwt::BitEncoder();
  destination_->Connect(out_);
  /* Add new probability models here: */
  pm_ = GiveProbabilityModel(prob_model);
}

/************************************************************************
 *                Global header for file                                *
 *----------------------------------------------------------------------*
 * Write- and ReadGlobalHeader defines global header and its format for *
 * the compressed file.                                                 *
 ************************************************************************/
void Encoder::WriteGlobalHeader(char preproc, char encoding) {
  /* At the moment dummy implementation. In future should use
   * bit-fields of a bytes as a flags. */
  out_->WriteByte(static_cast<byte>(preproc));
  out_->WriteByte(static_cast<byte>(encoding));
}

char Decoder::ReadGlobalHeader() {
  char preproc = static_cast<char>(in_->ReadByte());
  char probmodel = static_cast<char>(in_->ReadByte());
  pm_ = GiveProbabilityModel(probmodel);
  return preproc;
}
/***************** Global header for file-section ends ******************/

void Encoder::EncodeByte(byte b) {
  for(int i = 0; i < 8; ++i, b <<= 1) {
    bool bit = b & 0x80;
    destination_->Encode(bit, pm_->ProbabilityOfOne());
    pm_->Update(bit);
  }
}

void Encoder::EncodeRange(const byte* begin, const byte* end) {
  while(begin != end) {
    EncodeByte(*begin);
    ++begin;
  }
}

Encoder::~Encoder() {
  delete out_;
  delete destination_;
  delete pm_;
}

/*************************************************************************
 *            Encoding and decoding single MainBlock                     *
 *-----------------------------------------------------------------------*
 * Next functions handle encoding and decoding of the main blocks.       *
 * Also block header-format is specified here.                           *
 *************************************************************************/

// TODO: for compressing straight to the stdout we need to use
//       temporary file or huge buffer for each mainblock, so that
//       we can write the size of the compressed block in the beginning
// At the moment the implementation is done only for compressing into file
void Encoder::EncodeMainBlock(bwtc::MainBlock* block) {
  std::streampos len_pos = WriteBlockHeader(block->Stats());
}

std::streampos Encoder::WriteBlockHeader(uint64* stats) {
  std::streampos header_start = out_->GetPos();
  for (int i = 0; i < 6; ++i) out_->WriteByte(0x00); //fill 48 bits
  for (int i = 0; i < 256; ++i) {
    int bytes;
    if(stats[i]) {
      uint64 packed_cblock_size = PackInteger(stats[i], &bytes);
    }
  }
}

/*********** Encoding and decoding single MainBlock-section ends ********/

Decoder::Decoder(const std::string& source, char prob_model) :
    in_(NULL), source_(NULL), pm_(NULL) {
  in_ = new InStream(source);
  source_ = new dcsbwt::BitDecoder();
  source_->Connect(in_);
  pm_ = GiveProbabilityModel(prob_model);
}

Decoder::Decoder(const std::string& source) :
    in_(NULL), source_(NULL), pm_(NULL) {
  in_ = new InStream(source);
  source_ = new dcsbwt::BitDecoder();
  source_->Connect(in_);
}

Decoder::~Decoder() {
  delete in_;
  delete source_;
  delete pm_;
}

byte Decoder::DecodeByte() {
  byte b = 0x00;
  for(int i = 0; i < 8; ++i) {
    if (source_->Decode(pm_->ProbabilityOfOne())) {
      b |= 1;
      pm_->Update(true);
    } else {
      pm_->Update(false);
    }
    if (i < 7) b <<= 1;
  }
  return b;
}

} // namespace bwtc

