/**
 * @file Grammar.hpp
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
 * Header for grammar-object used to collect information about the
 * choices done during the preprocessing.
 */

#ifndef BWTC_GRAMMAR_HPP_
#define BWTC_GRAMMAR_HPP_

#include <algorithm>
#include <map>
#include <vector>

#include "../globaldefs.hpp"
#include "../Streams.hpp"

namespace bwtc {

/**Grammar object stores information about the replacements done
 * during the preprocessing. Preprocessors need grammar for storing the
 * special symbols and updating the right-hand sides of the rules if
 * needed (this may happen if some variable of grammar is promoted to
 * special symbol).
 */
class Grammar {
 private:
  /**PrRule stores single replacement. They are chosen by preprocessing
   * algoritms. This class is meant only for internal use of grammar.*/
  class PrRule {
   public:
    PrRule(uint16 variable, uint32 begin, uint32 end, bool largeVariable=false)
        : m_begin(begin), m_end(end), m_variable(variable),
          m_largeVariable(largeVariable) { }

    inline uint32 begin() const {return m_begin;}
    inline uint32 end() const {return m_end;}
    inline uint16 variable() const {return m_variable;}
    inline bool isLarge() const { return m_largeVariable; }
    inline void setRange(uint32 begin, uint32 end) {
      m_begin = begin;
      m_end = end;
    }
    inline void changeVariable(uint16 nVariable, bool large=true) {
      m_variable = nVariable;
      m_largeVariable = large;
    }
    inline size_t length() const { return m_end - m_begin; }

   private:
    /**Starting index in Grammar::m_rightHandSides of replaced string.*/
    uint32 m_begin;
    /**One past last index in Grammar::m_rightHandSides replaced string.*/
    uint32 m_end;
    uint16 m_variable;
    /**Variable is large if it's formed of two special symbols.
     * Otherwise the actual value of m_variable represents the actual
     * replacement character.*/
    bool m_largeVariable;
  };

 public:
  /**This class is meant to be used from the outside. 
   */
  class Rule {
   public:
    Rule(const PrRule& rule, const byte* start)
        : m_begin(start + rule.begin()), m_end(start + rule.end()),
          m_variable(rule.variable()), m_largeVariable(rule.isLarge())
    {}
    
    inline const byte *begin() const { return m_begin; }
    inline const byte *end() const { return m_end; }
    inline uint16 variable() const {return m_variable;}
    inline bool isLarge() const { return m_largeVariable; }
    inline size_t length() const { return m_end - m_begin; }

   private:
    const byte* const m_begin;
    const byte* const m_end;
    const uint16 m_variable;
    const bool m_largeVariable;
  }; 
  
  
  Grammar();

  inline bool isSpecial(byte symbol) const { return m_isSpecialSymbol[symbol]; }

  inline size_t numOfRules() const { return m_rules.size(); }
  
  /**Adding replacement for pair where symbols on the right side aren't freed.*/
  void addRule(byte variable, byte first, byte second);

  /**Adding replacement for longer string.*/
  void addRule(byte variable, byte *begin, size_t length);

  inline uint32 specialSymbolPairsLeft() const {
    uint32 s = m_specialSymbols.size();
    return s*s - m_specialPairReplacements.size();
  }

  void addSpecialSymbol(byte special);

  void printRules() const;
  
  inline uint32 numberOfRules() const {
    return m_rules.size();
  }

  inline uint32 numberOfSpecialSymbols() const {
    return m_specialSymbols.size();
  }
  
  inline const size_t* frequencies() const {
    return m_frequencies;
  }

  inline void beginUpdatingRules() {
    m_updatingRules = true;
    m_newRules = 0;
  }

  inline void endUpdatingRules(const std::vector<byte>& variables) {
    m_updatingRules = false;
    for(size_t i = 0; i < variables.size(); ++i)
      m_isVariable[variables[i]] = true;
  }

  inline void endUpdatingRules() {
    m_updatingRules = false;
  }

  inline Rule getRule(size_t index) const {
    return Rule(m_rules[index], &m_rightHandSides[0]);
  }

  inline bool isVariable(byte sym) const {
    return m_isVariable[sym];
  }
  
  uint16 specialPair(uint32 ord) const;
  
  /**The first entry in pair is special pair which is used for freeing the
   * second member. */
  void freedSymbols(std::vector<std::pair<uint16, byte> >& replacements) const;

  uint32 numberOfFreedSymbols() const;

  uint32 numberOfSpecialPair(uint32 first, uint32 snd) const;
  
  void expandAlphabet(const std::vector<byte>& freedSymbols,
                      const std::vector<byte>& newSpecials,
                      std::vector<uint16>& nextSpecialPairs);
  
  uint32 writeGrammar(OutStream* dst) const;
  uint32 writeNumberOfRules(OutStream* dst) const;
  uint32 writeLeftSides(OutStream* dst) const;
  uint32 writeVariableFlags(OutStream* dst) const;
  uint32 writeVariables(OutStream* dst) const;
  uint32 writeLengthsOfRules(OutStream* dst) const;
  uint32 writeNumberOfSpecialSymbols(OutStream* dst) const;
  uint32 writeSpecialSymbols(OutStream* dst) const;
  uint32 writeFreedSymbols(OutStream* dst) const;  
  uint32 writeRightSides(OutStream* dst) const;

  /**Grammar must be empty before it's read.*/
  void readGrammar(InStream* in);
  
 private:
  /**Frequencies of bytes in the right-side of rules.*/
  size_t m_frequencies[256];
  bool m_isSpecialSymbol[256];
  bool m_isVariable[256];

  std::vector<byte> m_specialSymbols;
  /**Special symbols are numbered in order they are created.
   * If pair.first==true then pair of this index is used as a grammar
   * variable. */
  std::vector<std::pair<bool,byte> > m_specialPairReplacements;

  std::vector<PrRule> m_rules;
  /**Right-hand sides of the rules.*/
  std::vector<byte> m_rightHandSides;

  /**Sould be equal to the numer of pairs in m_specialPairReplacements, where
   * first member is equal to true.*/
  uint32 m_specialSymbolsAsVariables;

  uint16 m_newRules;

  bool m_updatingRules;
  
};

} //namespace bwtc

#endif
