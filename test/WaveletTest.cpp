/**
 * @file WaveletTest.cpp
 * @author Pekka Mikkola <pjmikkol@cs.helsinki.fi>
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
 * Unit tests for wavelet tree.
 */

#define BOOST_TEST_MODULE 
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <cstring>
#include <iterator>
#include <utility>
#include <vector>

#include "../Utils.hpp"
#include "../WaveletTree.hpp"

namespace bwtc {
int verbosity = 0;

namespace tests {

BOOST_AUTO_TEST_SUITE(HeapTests)

BOOST_AUTO_TEST_CASE(HeapTest1) {
  MinimumHeap<int> heap;
  heap.insert(4, 99);
  heap.insert(18, 3);
  heap.insert(16, 77);
  BOOST_CHECK_EQUAL(heap.deleteMin().first, 18);
  BOOST_CHECK_EQUAL(heap.deleteMin().first, 16);
  BOOST_CHECK_EQUAL(heap.deleteMin().first, 4);
}

BOOST_AUTO_TEST_CASE(HeapTest2) {
  MinimumHeap<int> heap;
  heap.insert(4, 4);
  heap.insert(20, 5);
  heap.insert(16, 16);
  heap.insert(20, 20);
  heap.insert(4, 22);
  heap.insert(16, 17);
  BOOST_CHECK_EQUAL(heap.deleteMin().first, 4);
  BOOST_CHECK_EQUAL(heap.deleteMin().first, 20);
  BOOST_CHECK_EQUAL(heap.deleteMin().first, 16);
  BOOST_CHECK_EQUAL(heap.deleteMin().first, 16);
  BOOST_CHECK_EQUAL(heap.deleteMin().first, 20);
  BOOST_CHECK_EQUAL(heap.deleteMin().first, 4);
}


BOOST_AUTO_TEST_SUITE_END()

template <typename T, typename U>
void checkEqual(const T& vec, const U& answ) {
  for(size_t i = 0; i < vec.size(); ++i) {
    BOOST_CHECK_EQUAL(vec[i], answ[i]);
  }
}

BOOST_AUTO_TEST_SUITE(TreeConstructionTests)

template <typename BitVector>
int checkHuffmanShape(TreeNode<BitVector> *node, const char *answ, int *depths,
                      int curr, int depth)
{
  if(node->m_left == 0 && node->m_right == 0) {
    AlphabeticNode<BitVector>* n = dynamic_cast<AlphabeticNode<BitVector>*>(node);
    BOOST_CHECK_EQUAL(n->m_symbol, answ[curr]);
    BOOST_CHECK_EQUAL(depth, depths[curr]);
    return curr+1;
  }
  if(node->m_left) {
    curr = checkHuffmanShape(node->m_left, answ, depths, curr, depth+1);
  }
  if(node->m_right) {
    curr = checkHuffmanShape(node->m_right, answ, depths, curr, depth+1);
  }
  return curr;
}

BOOST_AUTO_TEST_CASE(HuffmanShape1) {
  uint64 freqs[256] = {0};
  freqs['a'] = 4;
  freqs['b'] = 2;
  freqs['c'] = 1;
  TreeNode<std::vector<bool> > *root =
      WaveletTree<std::vector<bool> >::createHuffmanShape(freqs);
  const char *answers = "abc";
  int depths[] = {1,2,2};
  checkHuffmanShape(root, answers, depths, 0, 0);

  std::vector<bool> codes[256];
  WaveletTree<std::vector<bool> >::collectCodes(codes, root);
  bool aCode[] = {false};
  bool bCode[] = {true, false};
  bool cCode[] = {true, true};
  checkEqual(codes['a'], aCode);
  checkEqual(codes['b'], bCode);
  checkEqual(codes['c'], cCode);
}

BOOST_AUTO_TEST_CASE(HuffmanShape2) {
  uint64 freqs[256] = {0};
  freqs['c'] = 4;
  freqs['b'] = 5;
  freqs['a'] = 6;
  freqs['d'] = 20;
  TreeNode<std::vector<bool> > *root =
      WaveletTree<std::vector<bool> >::createHuffmanShape(freqs);
  const char *answers = "dbca";
  int depths[] = {1, 3, 3, 2};  
  checkHuffmanShape(root, answers, depths, 0, 0);

  std::vector<bool> codes[256];
  WaveletTree<std::vector<bool> >::collectCodes(codes, root);
  bool dCode[] = {false};
  bool bCode[] = {true, false, false};
  bool cCode[] = {true, false, true};
  bool aCode[] = {true, true};
  checkEqual(codes['d'], dCode);
  checkEqual(codes['b'], bCode);
  checkEqual(codes['c'], cCode);
  checkEqual(codes['a'], aCode);
}

BOOST_AUTO_TEST_CASE(HuffmanShape3) {
  uint64 freqs[256] = {0};
  const char *str = "baaabaaabcb";
  utils::calculateRunFrequencies(freqs, (const byte*) str, strlen(str));
  TreeNode<std::vector<bool> > *root =
      WaveletTree<std::vector<bool> >::createHuffmanShape(freqs);
  const char *answers = "bac";
  int depths[] = {1, 2, 2};
  checkHuffmanShape(root, answers, depths, 0, 0);

  std::vector<bool> codes[256];
  WaveletTree<std::vector<bool> >::collectCodes(codes, root);
  bool bCode[] = {false};
  bool aCode[] = {true, false};
  bool cCode[] = {true, true};
  checkEqual(codes['a'], aCode);
  checkEqual(codes['b'], bCode);
  checkEqual(codes['c'], cCode);
}

BOOST_AUTO_TEST_CASE(HuffmanShape4) {
  uint64 freqs[256] = {0};
  const char *str = "aaaa";
  utils::calculateRunFrequencies(freqs, (const byte*) str, strlen(str));
  TreeNode<std::vector<bool> > *root =
      WaveletTree<std::vector<bool> >::createHuffmanShape(freqs);
  const char *answers = "a";
  int depths[] = {1};
  checkHuffmanShape(root, answers, depths, 0, 0);

  std::vector<bool> codes[256];
  WaveletTree<std::vector<bool> >::collectCodes(codes, root);
  bool aCode[] = {false};
  checkEqual(codes['a'], aCode);
}

BOOST_AUTO_TEST_CASE(WholeConstruction1) {
  const char *str = "aaabbaaacbcb";
  WaveletTree<std::vector<bool> > tree((const byte*) str, strlen(str));
  std::vector<byte> msg;
  tree.message(std::back_inserter(msg));
  BOOST_CHECK_EQUAL(msg.size(), strlen(str));
  checkEqual(msg, (const byte*) str);
}

BOOST_AUTO_TEST_CASE(WholeConstruction2) {
  const char *str = "abbbabaagggffllslwerkfdskofdsksasdadsasdfgdfsmldsgklmesgfklmfeeeeeeeeeg";
  WaveletTree<std::vector<bool> > tree((const byte*) str, strlen(str));
  std::vector<byte> msg;
  tree.message(std::back_inserter(msg));
  BOOST_CHECK_EQUAL(msg.size(), strlen(str));
  checkEqual(msg, (const byte*) str);
}

BOOST_AUTO_TEST_CASE(WholeConstruction3) {
  const char *str = "aaaaaaaaaaaaaac";
  WaveletTree<std::vector<bool> > tree((const byte*) str, strlen(str));
  std::vector<byte> msg;
  tree.message(std::back_inserter(msg));
  BOOST_CHECK_EQUAL(msg.size(), strlen(str));
  checkEqual(msg, (const byte*) str);
}

BOOST_AUTO_TEST_CASE(WholeConstruction4) {
  const char *str = "aaaaaa"; 
  WaveletTree<std::vector<bool> > tree((const byte*) str, strlen(str));
  std::vector<byte> msg;
  tree.message(std::back_inserter(msg));
  BOOST_CHECK_EQUAL(msg.size(), strlen(str));
  checkEqual(msg, (const byte*) str);
}

BOOST_AUTO_TEST_CASE(WholeConstruction5) {
  const char *str = "abcdefghijklmnababcabcdabcdeabcdefacbcdefgabcdefghabcdefghiabcdefghij";
  WaveletTree<std::vector<bool> > tree((const byte*) str, strlen(str));
  std::vector<byte> msg;
  tree.message(std::back_inserter(msg));
  BOOST_CHECK_EQUAL(msg.size(), strlen(str));
  checkEqual(msg, (const byte*) str);
}

BOOST_AUTO_TEST_CASE(WholeConstruction6) {
  const char *str = "abaabaaabaaaabaaaaabaaaaaabaaaaaaaabaaaaaaaaaaaa";
  WaveletTree<std::vector<bool> > tree((const byte*) str, strlen(str));
  std::vector<byte> msg;
  tree.message(std::back_inserter(msg));
  BOOST_CHECK_EQUAL(msg.size(), strlen(str));
  checkEqual(msg, (const byte*) str);
}


BOOST_AUTO_TEST_SUITE_END()



BOOST_AUTO_TEST_SUITE(WaveletTreeShape)

struct Input {
  Input() : bitsRead(0) {}

  std::vector<bool> bits;
  size_t bitsRead;
  bool readBit() { return bits.at(bitsRead++); }
};

BOOST_AUTO_TEST_CASE(ShapeEncoding1) {
  const char *str = "aaaaaaaa";
  WaveletTree<std::vector<bool> > tree((const byte*) str, strlen(str));
  std::vector<bool> shapeVec;
  tree.treeShape(shapeVec);
  BOOST_CHECK_EQUAL(shapeVec.size(), 257);
  for(size_t i = 0; i < 257; ++i) {
    if(i == 'a') {
      BOOST_CHECK_EQUAL(shapeVec[i], true);
    } else {
      BOOST_CHECK_EQUAL(shapeVec[i], false);
    }
  }
}

BOOST_AUTO_TEST_CASE(ShapeEncoding2) {
  const char *str = "ahahabahbahaeaeabeabababa";
  WaveletTree<std::vector<bool> > tree((const byte*) str, strlen(str));
  std::vector<bool> expected;
  for(size_t i = 0; i < 256; ++i) {
    if(i == 'a' || i == 'b' || i == 'h' || i == 'e') {
      expected.push_back(true);
    } else {
      expected.push_back(false);
    }
  }
  // root
  expected.push_back(true); expected.push_back(false); expected.push_back(false); 
  expected.push_back(false);
  // left child of the root
  expected.push_back(true); expected.push_back(false); expected.push_back(false); 
  // left child of the previous node
  expected.push_back(true); expected.push_back(false);

  std::vector<bool> shapeVec;
  tree.treeShape(shapeVec);

  BOOST_CHECK_EQUAL(shapeVec.size(), 265);
  checkEqual(shapeVec, expected);
}

BOOST_AUTO_TEST_CASE(ShapeEncoding3) {
  const char *str = "abcdabcdabcdabcaba";
  WaveletTree<std::vector<bool> > tree((const byte*) str, strlen(str));
  std::vector<bool> expected;
  for(size_t i = 0; i < 256; ++i) {
    if(i == 'a' || i == 'b' || i == 'c' || i == 'd') {
      expected.push_back(true);
    } else {
      expected.push_back(false);
    }
  }
  // root
  expected.push_back(false); expected.push_back(false); expected.push_back(true); 
  expected.push_back(true);
  // left child of the root
  expected.push_back(false); expected.push_back(true);
  // right child of the root
  expected.push_back(false); expected.push_back(true);

  std::vector<bool> shapeVec;
  tree.treeShape(shapeVec);

  BOOST_CHECK_EQUAL(shapeVec.size(), 264);
  checkEqual(shapeVec, expected);
}

BOOST_AUTO_TEST_CASE(ShapeDecoding1) {
  Input input;
  WaveletTree<std::vector<bool> > tree;

  for(size_t i = 0; i < 257; ++i) {
    if(i == 'a') {
      input.bits.push_back(true);
    } else {
      input.bits.push_back(false);
    }
  }
  size_t bits = tree.readShape(input);
  
  BOOST_CHECK_EQUAL(input.bitsRead, 257);
  BOOST_CHECK_EQUAL(bits, 257);
  BOOST_CHECK_EQUAL(tree.code('a').size(), 1);
  BOOST_CHECK_EQUAL(tree.code('a')[0], false);
  
}

BOOST_AUTO_TEST_CASE(ShapeDecoding2) {
  Input input;
  for(size_t i = 0; i < 256; ++i) {
    if(i == 'a' || i == 'b' || i == 'h' || i == 'e') {
      input.bits.push_back(true);
    } else {
      input.bits.push_back(false);
    }
  }
  // root
  input.bits.push_back(true); input.bits.push_back(false);
  input.bits.push_back(false); input.bits.push_back(false);
  // left child of the root
  input.bits.push_back(true); input.bits.push_back(false);
  input.bits.push_back(false); 
  // left child of the previous node
  input.bits.push_back(true); input.bits.push_back(false);

  WaveletTree<std::vector<bool> > tree;
  size_t bits = tree.readShape(input);

  BOOST_CHECK_EQUAL(bits, 265);
  bool aCode[] = {true};
  bool bCode[] = {false, true};
  bool hCode[] = {false, false, false};
  bool eCode[] = {false, false, true};
  checkEqual(tree.code('a'), aCode);
  checkEqual(tree.code('b'), bCode);
  checkEqual(tree.code('h'), hCode);
  checkEqual(tree.code('e'), eCode);
}

BOOST_AUTO_TEST_CASE(ShapeDecoding3) {
  Input input;
  for(size_t i = 0; i < 256; ++i) {
    if(i == 'a' || i == 'b' || i == 'c' || i == 'd') {
      input.bits.push_back(true);
    } else {
      input.bits.push_back(false);
    }
  }

  // root
  input.bits.push_back(false); input.bits.push_back(false);
  input.bits.push_back(true); input.bits.push_back(true);
  // left child of the root
  input.bits.push_back(false); input.bits.push_back(true);
  // right child of the root
  input.bits.push_back(false); input.bits.push_back(true);

  WaveletTree<std::vector<bool> > tree;
  size_t bits = tree.readShape(input);

  BOOST_CHECK_EQUAL(input.bitsRead, 264);
  BOOST_CHECK_EQUAL(bits, 264);
  
  bool aCode[] = {false, false};
  bool bCode[] = {false, true};
  bool cCode[] = {true, false};
  bool dCode[] = {true, true};
  checkEqual(tree.code('a'), aCode);
  checkEqual(tree.code('b'), bCode);
  checkEqual(tree.code('c'), cCode);
  checkEqual(tree.code('d'), dCode);
}



BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(GammaCodes)

BOOST_AUTO_TEST_CASE(Construction1) {
  std::vector<bool> one, five, seven, fifty;
  WaveletTree<std::vector<bool> >::gammaCode(one, 1);
  BOOST_CHECK_EQUAL(one.size(), 1);
  WaveletTree<std::vector<bool> >::gammaCode(five, 5);
  BOOST_CHECK_EQUAL(five.size(), 5);
  WaveletTree<std::vector<bool> >::gammaCode(seven, 7);
  BOOST_CHECK_EQUAL(seven.size(), 5);
  WaveletTree<std::vector<bool> >::gammaCode(fifty, 50);
  BOOST_CHECK_EQUAL(fifty.size(), 11);

  bool oneCode[] = {false};
  bool fiveCode[] = {true, true, false, false, true};
  bool sevenCode[] = {true, true, false, true, true};
  bool fiftyCode[] = {true, true, true, true, true,
                      false, true, false, false, true, false};
  checkEqual(one, oneCode);
  checkEqual(five, fiveCode);
  checkEqual(seven, sevenCode);
  checkEqual(fifty, fiftyCode);
}

BOOST_AUTO_TEST_SUITE_END()


} //namespace tests
} //namespace long_sequences

