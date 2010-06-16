#ifndef BWTC_PREPROCESSOR_H_
#define BWTC_PREPROCESSOR_H_

#include <iostream> /* for std::streamsize*/
#include <string>

#include "block_manager.h"
#include "globaldefs.h"
#include "stream.h"

namespace bwtc {

//TODO: Make this class to abstract class (interface for real implementations)
class PreProcessor {
 public:
  PreProcessor(uint64 block_size);
  ~PreProcessor();
  void Connect(std::string source_name);
  void AddBlockManager(BlockManager* bm);
  /* Reads and preprocesses data to byte array provided by block_manager_*/
  MainBlock* ReadBlock();

 private:
  InStream* source_;
  uint64 block_size_;
  BlockManager* block_manager_;

  /* This should be done during preprocessing*/
  void BuildStats(byte* data, std::vector<uint64>* stats, uint64 size);
  PreProcessor& operator=(const PreProcessor& p);
  PreProcessor(const PreProcessor&);
};

/* This function returns chosen preprocessor */ 
PreProcessor* GivePreProcessor(
    char choice, uint64 block_size, const std::string& input);

} // namespace bwtc

#endif