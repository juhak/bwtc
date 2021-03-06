/**
 * @file WaveletCoders.cpp
 * @author Pekka Mikkola <pmikkol@gmail.com>
 *
 * @section LICENSE
 *
 * This file is part of bwtc.
 *
 * bwtc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bwtc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bwtc.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @section DESCRIPTION
 *
 * Implementations for the wavelet-decoder and -encoder.
 */

#include <cassert>

#include <iterator>
#include <iostream> // For std::streampos
#include <numeric> // for std::accumulate
#include <string>
#include <vector>

#include "WaveletCoders.hpp"
#include "globaldefs.hpp"
#include "Utils.hpp"
#include "probmodels/ProbabilityModel.hpp"
#include "WaveletTree.hpp"
#include "Profiling.hpp"

namespace bwtc {

WaveletEncoder::WaveletEncoder(char prob_model)
    : m_probModel(giveProbabilityModel(prob_model)),
      m_integerProbModel(giveModelForIntegerCodes()),
      m_gapProbModel(giveModelForGaps()),
      m_headerPosition(0), m_compressedBlockLength(0)
{
#ifdef ENTROPY_PROFILER
  m_bytesForCharacters = 0;
  m_bytesForRuns = 0;
#endif
}

WaveletEncoder::~WaveletEncoder() {
  delete m_probModel;
  delete m_integerProbModel;
  delete m_gapProbModel;
}

void WaveletEncoder::endContextBlock() {
  assert(m_probModel);
  m_probModel->resetModel();
  m_integerProbModel->resetModel();
  m_gapProbModel->resetModel();
  m_destination.finish();
}

void WaveletDecoder::endContextBlock() {
  assert(m_probModel);
  m_probModel->resetModel();
  m_integerProbModel->resetModel();
  m_gapProbModel->resetModel();
}

size_t WaveletEncoder::
transformAndEncode(BWTBlock& block, BWTManager& bwtm, OutStream* out) {
  std::vector<uint32> characterFrequencies(256, 0);
  bwtm.doTransform(block, &characterFrequencies[0]);

  m_destination.connect(out);
  writeBlockHeader(block, characterFrequencies, out);
  encodeData(block.begin(), characterFrequencies, out);
  finishBlock(out);
  return m_compressedBlockLength + 6; //Also bytes for the compressedSize
}


/******************************************************************************
 *            Encoding and decoding single MainBlock                          *
 *----------------------------------------------------------------------------*
 * Following functions handle encoding and decoding of the main blocks.       *
 * Also block header-format is specified here.                                *
 *                                                                            *
 * Format of the block is following:                                          *
 *  - Block header (no fixed length)                              (1)         *
 *  - Compressed block (no fixed length)                          (2)         *
 *  - Block trailer (coded same way as the context block lengths) (3)         *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *                                                                            *
 * Block header (1) format is following:                                      *
 * a) Length of the (header + compressed block + trailer) in bytes (6 bytes). *
 *    Note that the the length field itself isn't included in total length    *
 * b) List of context block lengths. Lengths are compressed with              *
 *    utils::PackInteger-function.                                            *
 ******************************************************************************/

//At the moment we lose at worst case 7 bits when writing the shape of
//wavelet tree
void WaveletEncoder::
encodeData(const byte* block, const std::vector<uint32>& stats, OutStream* out)
{
  PROFILE("WaveletEncoder::encodeData");
  size_t beg = 0;
  for(size_t i = 0; i < stats.size(); ++i) {
    if(stats[i] == 0) continue;
    WaveletTree<std::vector<bool> > wavelet(&block[beg], stats[i]);

    int bytes;
    writePackedInteger(utils::packInteger(wavelet.bitsInRoot(), &bytes), out);
    m_compressedBlockLength += bytes;

    std::vector<bool> shape;
    wavelet.treeShape(shape);

    // Write shape vector to output
    for(size_t k = 0; k < shape.size();) {
      byte b = 0; size_t j = 0;
      for(; j < 8 && k < shape.size(); ++k, ++j) {
        b <<= 1;
        b |= (shape[k])?1:0;
      }
      if (j < 8) b <<= (8-j);
      out->writeByte(b);
      ++m_compressedBlockLength;
    }
    if(verbosity > 3) {
      size_t shapeBytes = shape.size()/8;
      if(shape.size()%8 > 0) ++shapeBytes;
      std::clog << "Shape of wavelet tree took " << shapeBytes << " bytes.\n";
      std::clog << "Wavelet tree takes " << wavelet.totalBits()
                << " bits in total\n";
    }
    wavelet.encodeTreeBF(m_destination, *m_probModel, *m_integerProbModel,
                         *m_gapProbModel);

#ifdef ENTROPY_PROFILER    
    m_bytesForCharacters += wavelet.m_bytesForCharacters;
    m_bytesForRuns += wavelet.m_bytesForRuns;
#endif
    
    beg += stats[i];
    endContextBlock();
  }
}

void WaveletEncoder::
finishBlock(OutStream* out) {
  m_compressedBlockLength += m_destination.counter();
  out->write48bits(m_compressedBlockLength, m_headerPosition);
}

/*********************************************************************
 * The format of header for single main block is the following:      *
 * - 48 bits for the length of the compressed main block, doesn't    *
 *   include 6 bytes used for this                                   *
 * - byte representing the number of separately encoded sections.    *
 *   zero represents 256                                             *
 * - lengths of the sections which are encoded with same wavelet tree*
 *********************************************************************/
void WaveletEncoder::
writeBlockHeader(const BWTBlock& block, std::vector<uint32>& stats,
                 OutStream* out) {
  uint64 headerLength = 0;
  m_headerPosition = out->getPos();
  for (unsigned i = 0; i < 6; ++i) out->writeByte(0x00); //fill 48 bits

  headerLength += block.writeHeader(out);
  
  /* Deduce sections for separate encoding. At the moment uses not-so-well
   * thought heuristic. */
  std::vector<uint32> temp; std::vector<uint32>& s = stats;
  size_t sum = 0;
  for(size_t i = 0; i < s.size(); ++i) {
    sum += s[i];
    if(sum >= 10000) {
      temp.push_back(sum);
      sum = 0;
    }
  }
  if (sum != 0) {
    if(temp.size() > 0) temp.back() += sum;
    else temp.push_back(sum);
  }
  s.resize(temp.size());
  std::copy(temp.begin(), temp.end(), s.begin());
  byte len;
  if(temp.size() == 256) len = 0;
  else len = temp.size();
  out->writeByte(len);
  headerLength += 1;

  assert(s.size() == temp.size());
  assert(temp.size() <= 256);

  for (size_t i = 0; i < stats.size(); ++i) {
    int bytes;
    uint64 packed_cblock_size = utils::packInteger(stats[i], &bytes);
    headerLength += bytes;
    writePackedInteger(packed_cblock_size, out);
  }
  // TODO: Calculate Runs and their coding
  
  m_compressedBlockLength = headerLength;

  m_destination.resetCounter();
}

/* Integer is written in reversal fashion so that it can be read easier.*/
void WaveletEncoder::
writePackedInteger(uint64 packed_integer, OutStream* out) {
  do {
    byte to_written = static_cast<byte>(packed_integer & 0xFF);
    packed_integer >>= 8;
    out->writeByte(to_written);
  } while (packed_integer);
}

uint64
WaveletDecoder::readBlockHeader(BWTBlock& block, std::vector<uint64>* stats,
                                InStream* in) {
  uint64 compressed_length = in->read48bits();
  block.readHeader(in);
  
  byte sections = in->readByte();
  size_t sects = (sections == 0) ? 256 : sections;
  for(size_t i = 0; i < sects; ++i) {
    uint64 value = readPackedInteger(in);
    stats->push_back(utils::unpackInteger(value));
  }
  return compressed_length;
}

void WaveletDecoder::decodeBlock(BWTBlock& block, InStream* in) {
  PROFILE("WaveletDecoder::decodeBlock");
  if(in->compressedDataEnding()) return;

  std::vector<uint64> context_lengths;
  uint64 compr_len = readBlockHeader(block, &context_lengths, in);

  if (verbosity > 2) {
    std::clog << "Size of compressed block = " << compr_len << "\n";
  }

#ifndef NDEBUG
  uint64 blockSize = std::accumulate(
      context_lengths.begin(), context_lengths.end(), static_cast<uint64>(0));
#endif

  m_source.connect(in);
  
  size_t len = 0;
  for(size_t i = 0; i < context_lengths.size(); ++i) {
    if(context_lengths[i] == 0) continue;
    size_t rootSize = utils::unpackInteger(readPackedInteger(in));

    WaveletTree<std::vector<bool> > wavelet;

    size_t bits = wavelet.readShape(*in);

    in->flushBuffer();
    m_source.start();
    wavelet.decodeTreeBF(rootSize, m_source, *m_probModel, *m_integerProbModel,
                         *m_gapProbModel);
    if(verbosity > 3) {
      size_t shapeBytes = bits/8;
      if(bits%8 > 0) ++shapeBytes;
      std::clog << "Shape of wavelet tree took " << shapeBytes << " bytes.\n";
      std::clog << "Wavelet tree takes " << wavelet.totalBits()
                << " bits in total\n";
    }

    size_t clen = wavelet.message(block.begin() + len);
    len += clen;
    endContextBlock();
  }
  block.setSize(len);
  assert(len == blockSize);
}

uint64 WaveletDecoder::readPackedInteger(InStream* in) {
  static const uint64 kEndSymbol = static_cast<uint64>(1) << 63;
  static const uint64 kEndMask = static_cast<uint64>(1) << 7;

  uint64 packed_integer = 0;
  bool bits_left = true;
  int i;
  for(i = 0; bits_left; ++i) {
    uint64 read = static_cast<uint64>(in->readByte());
    bits_left = (read & kEndMask) != 0;
    packed_integer |= (read << i*8);
  }
  if (packed_integer == 0x80) return kEndSymbol;
  return packed_integer;
}
/*********** Encoding and decoding single MainBlock-section ends ********/

WaveletDecoder::WaveletDecoder() :
    m_probModel(0), m_integerProbModel(giveModelForIntegerCodes()),
    m_gapProbModel(giveModelForGaps())
{}

WaveletDecoder::WaveletDecoder(char decoder) :
    m_probModel(giveProbabilityModel(decoder)),
    m_integerProbModel(giveModelForIntegerCodes()),
    m_gapProbModel(giveModelForGaps())
{}

WaveletDecoder::~WaveletDecoder() {
  delete m_probModel;
  delete m_integerProbModel;
  delete m_gapProbModel;
}


} // namespace bwtc

