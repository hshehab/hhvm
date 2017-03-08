/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-present Facebook, Inc. (http://www.facebook.com)  |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/
#ifndef incl_HHBBC_ARRAY_LIKE_MAP_H_
#define incl_HHBBC_ARRAY_LIKE_MAP_H_

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>

#include "hphp/hhbbc/misc.h"
#include "hphp/runtime/base/typed-value.h"

namespace HPHP { namespace HHBBC {

struct Type;

/*
 * This is an insertion-order preserving hash map. Its used to emulate
 * the behavior of php and hack arrays in hhbbc. Handling of int-like
 * string keys must be done by the user.
 */
template <class K, class V = Type> struct ArrayLikeMap {
  using value_type = std::pair<K, V>;
  struct Hash {
    size_t operator()(const Cell& c) const { return c.m_data.num; }
    size_t operator()(LSString s) const { return (size_t)s.get(); }
    size_t operator()(int64_t i) const { return i; }
  };

  struct Equal {
    bool operator()(const Cell& c1, const Cell& c2) const {
      return c1.m_type == c2.m_type && c1.m_data.num == c2.m_data.num;
    }
    bool operator()(SString s, const Cell& c) const {
      return isStringType(c.m_type) && c.m_data.pstr == s;
    }
    bool operator()(int64_t i, const Cell& c) const {
      return c.m_type == KindOfInt64 && c.m_data.num == i;
    }
    bool operator()(LSString s1, LSString s2) const {
      return s1 == s2;
    }
    bool operator()(int64_t i1, int64_t i2) const {
      return i1 == i2;
    }
  };
private:
  using extractor = boost::multi_index::member<
    value_type, K, &value_type::first>;
  struct List {};
  struct Unordered {};
  using map = boost::multi_index::multi_index_container<
    value_type,
    boost::multi_index::indexed_by<
      boost::multi_index::sequenced<boost::multi_index::tag<List>>,
      boost::multi_index::hashed_unique<boost::multi_index::tag<Unordered>,
                                        extractor, Hash, Equal>>>;
  using unordered_index = typename boost::multi_index::index<
    map, Unordered>::type;
  using list_index = typename boost::multi_index::index<map, List>::type;
 public:
  using iterator = typename list_index::iterator;
  using const_iterator = typename list_index::const_iterator;

  iterator find(const K& k) {
    return m_map.template project<0>(getUnordered().find(k));
  }

  const_iterator find(const K& k) const {
    return m_map.template project<0>(getUnordered().find(k));
  }

  const_iterator find(SString k) const {
    return m_map.template project<0>(getUnordered().find(k));
  }

  const_iterator find(int64_t k) const {
    return m_map.template project<0>(getUnordered().find(k));
  }

  void update(iterator it, const V& v) {
    const_cast<V&>(it->second) = v;
  }

  void update(iterator it, V&& v) {
    const_cast<V&>(it->second) = std::move(v);
  }

  V& operator[](const K&k) {
    // emplace_back won't insert a new entry if the key already exists
    return const_cast<V&>(emplace_back(k, V{}).first->second);
  }

  std::pair<iterator, bool> emplace_back(const K& k, const V& v) {
    return getList().push_back({k, v});
  }

  std::pair<iterator, bool> emplace_front(const K& k, const V& v) {
    return getList().push_front({k, v});
  }

  iterator begin() { return getList().begin(); }
  iterator end()   { return getList().end(); }
  const_iterator begin() const { return getList().begin(); }
  const_iterator end() const { return getList().end(); }
  friend iterator begin(ArrayLikeMap& m) { return m.begin(); }
  friend iterator end(ArrayLikeMap& m) { return m.end(); }
  friend const_iterator begin(const ArrayLikeMap& m) { return m.begin(); }
  friend const_iterator end(const ArrayLikeMap& m) { return m.end(); }
  friend bool operator==(const ArrayLikeMap& a, const ArrayLikeMap& b) {
    if (a.size() != b.size()) return false;
    auto it = b.begin();
    for (auto& kv : a) {
      if (!Equal{}(kv.first, it->first)) return false;
      if (kv.second != it->second) return false;
      ++it;
    }
    return true;
  }
  bool empty() const { return m_map.empty(); }
  size_t size() const { return m_map.size(); }

  void clear() { m_map = {}; }

 private:
  unordered_index& getUnordered() { return m_map.template get<Unordered>(); }
  const unordered_index& getUnordered() const {
    return m_map.template get<Unordered>();
  }
  list_index& getList() { return m_map.template get<List>(); }
  const list_index& getList() const { return m_map.template get<List>(); }

  map m_map;
};

} }

#endif
