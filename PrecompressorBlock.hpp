/**
 * @file PrecompressorBlock.hpp
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
 * Header for Precompressor-block. The data of precompressor-block is first
 * read from the input stream. After precompression it is divided into
 * BWTBlocks which are then transformed and compressed independently.
 */

#ifndef BWTC_PRECOMPRESSORBLOCK_HPP_
#define BWTC_PRECOMPRESSORBLOCK_HPP_

#include "globaldefs.hpp"

#include <vector>

namespace bwtc {

class PrecompressorBlock {
 public:

 private:
  std::vector<byte> m_data;
};

} //namespace bwtc

#endif