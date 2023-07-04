#ifdef RYML_SINGLE_HEADER
#include "ryml_all.hpp"
#else
#include "c4/yml/new_parser.hpp"
#include "c4/yml/std/string.hpp"
#include "c4/yml/emit.hpp"
#include "c4/yml/detail/print.hpp"
#endif
#include <gtest/gtest.h>
#include "./callbacks_tester.hpp"


namespace c4 {
namespace yml {

struct EventSink
{
    std::string result;
    void operator() (csubstr s) { result.append(s.str, s.len); }
    void operator() (char c) { result += c; }
};

using PsEvents = NewParser<true, EventSink>;
using PsTree = NewParser<false, EventSink>;

template<template<class> class Fn>
void test_new_parser_events(std::string events)
{
    EventSink sink;
    PsEvents ps(&sink);
    Fn<PsEvents> fn;
    fn(ps);
    EXPECT_EQ(sink.result, events);
}

template<template<class> class Fn>
void test_new_parser_wtree(std::string yaml)
{
    Tree t = {};
    PsTree ps(&t);
    Fn<PsTree> fn;
    fn(ps);
    print_tree(t);
    std::string result = emitrs_yaml<std::string>(t);
    printf("~~~\n%s~~~\n", result.c_str());
    EXPECT_EQ(result, yaml);
}


//-----------------------------------------------------------------------------

#define PSTEST(name, yaml, events, ...)         \
template<class Ps>                              \
struct name                                     \
{                                               \
    void operator() (Ps &ps)                    \
    {                                           \
        __VA_ARGS__                             \
    }                                           \
};                                              \
TEST(NewParser, name##_events)                  \
{                                               \
    test_new_parser_events<name>(events);       \
}                                               \
TEST(NewParser, name##_wtree)                   \
{                                               \
    test_new_parser_wtree<name>(yaml);          \
}


#define ___                                                             \
    do                                                                  \
    {                                                                   \
       if(ps.is_wtree)                                                  \
       {                                                                \
           printf("%s:%d: parent.id=%zu curr.id=%zu\n",                 \
                  __FILE__, __LINE__, ps.m_parent->id, ps.m_curr.id);   \
       }                                                                \
    } while(0);


//-----------------------------------------------------------------------------

PSTEST(SimpleScalar,
       "foo\n",
        R"(+STR
+DOC
=VAL :foo
-DOC
-STR
)",
       ps._begin_stream();     ___
       ps._begin_doc();     ___
       ps._add_val_scalar_plain("foo");     ___
       ps._end_doc();     ___
       ps._end_stream();     ___
    )


//-----------------------------------------------------------------------------

PSTEST(SimpleMapFlow,
       "{foo: bar,foo2: bar2}",
        R"(+STR
+DOC
+MAP {}
=VAL :foo
=VAL :bar
=VAL :foo2
=VAL :bar2
-MAP
-DOC
-STR
)",
       PsTree::state st_map;
       ps._begin_stream();     ___
       ps._begin_doc();     ___
       ps._begin_map_val_flow(st_map);     ___
       ps._add_key_scalar_plain("foo");     ___
       ps._add_val_scalar_plain("bar");     ___
       ps._add_key_scalar_plain("foo2");     ___
       ps._add_val_scalar_plain("bar2");     ___
       ps._end_map();     ___
       ps._end_doc();     ___
       ps._end_stream();     ___
    )


//-----------------------------------------------------------------------------

PSTEST(SimpleMapBlock,
       "foo: bar\nfoo2: bar2\nfoo3: bar3\n",
       R"(+STR
+DOC
+MAP
=VAL :foo
=VAL :bar
=VAL :foo2
=VAL :bar2
=VAL :foo3
=VAL :bar3
-MAP
-DOC
-STR
)",
       PsTree::state st_map;
       ps._begin_stream();     ___
       ps._begin_doc();     ___
       ps._begin_map_val_block(st_map);     ___
       ps._add_key_scalar_plain("foo");     ___
       ps._add_val_scalar_plain("bar");     ___
       ps._add_key_scalar_plain("foo2");     ___
       ps._add_val_scalar_plain("bar2");     ___
       ps._add_key_scalar_plain("foo3");     ___
       ps._add_val_scalar_plain("bar3");     ___
       ps._end_map();     ___
       ps._end_doc();     ___
       ps._end_stream();     ___
    )


//-----------------------------------------------------------------------------

PSTEST(SimpleSeqFlow,
       "[foo,bar,baz]",
        R"(+STR
+DOC
+SEQ []
=VAL :foo
=VAL :bar
=VAL :baz
-MAP
-DOC
-STR
)",
       PsTree::state st_seq;
       ps._begin_stream();     ___
       ps._begin_doc();     ___
       ps._begin_seq_val_flow(st_seq);     ___
       ps._add_val_scalar_plain("foo");     ___
       ps._add_val_scalar_plain("bar");     ___
       ps._add_val_scalar_plain("baz");     ___
       ps._end_map();     ___
       ps._end_doc();     ___
       ps._end_stream();     ___
    )


//-----------------------------------------------------------------------------

PSTEST(SimpleSeqBlock,
       "- foo\n- bar\n- baz\n",
        R"(+STR
+DOC
+SEQ
=VAL :foo
=VAL :bar
=VAL :baz
-MAP
-DOC
-STR
)",
       PsTree::state st_seq;
       ps._begin_stream();     ___
       ps._begin_doc();     ___
       ps._begin_seq_val_block(st_seq);     ___
       ps._add_val_scalar_plain("foo");     ___
       ps._add_val_scalar_plain("bar");     ___
       ps._add_val_scalar_plain("baz");     ___
       ps._end_map();     ___
       ps._end_doc();     ___
       ps._end_stream();     ___
    )


//-----------------------------------------------------------------------------

PSTEST(MapMapFlow,
       "{map1: {foo1: bar1,FOO1: BAR1},map2: {foo2: bar2,FOO2: BAR2}}\n",
       R"(+STR
+DOC
+MAP {}
=VAL :map1
+MAP {}
=VAL :foo1
=VAL :bar1
=VAL :FOO1
=VAL :BAR1
-MAP
=VAL :map2
+MAP {}
=VAL :foo2
=VAL :bar2
=VAL :FOO2
=VAL :BAR2
-MAP
-MAP
-DOC
-STR
)",
       PsTree::state st_map, st_mapmap;
       ps._begin_stream();     ___
       ps._begin_doc();     ___
       ps._begin_map_val_flow(st_map);     ___
       ps._add_key_scalar_plain("map1");     ___
       ps._begin_map_val_flow(st_mapmap);     ___
       ps._add_key_scalar_plain("foo1");     ___
       ps._add_val_scalar_plain("bar1");     ___
       ps._add_key_scalar_plain("FOO1");     ___
       ps._add_val_scalar_plain("BAR1");     ___
       ps._end_map();     ___
       ps._add_key_scalar_plain("map2");     ___
       ps._begin_map_val_flow(st_mapmap);     ___
       ps._add_key_scalar_plain("foo2");     ___
       ps._add_val_scalar_plain("bar2");     ___
       ps._add_key_scalar_plain("FOO2");     ___
       ps._add_val_scalar_plain("BAR2");     ___
       ps._end_map();     ___
       ps._end_map();     ___
       ps._end_doc();     ___
       ps._end_stream();     ___
    )


//-----------------------------------------------------------------------------

PSTEST(MapMapBlock,
       R"(map1:
  foo1: bar1
  FOO1: BAR1
map2:
  foo2: bar2
  FOO2: BAR2
)",
       R"(+STR
+DOC
+MAP
=VAL :map1
+MAP
=VAL :foo1
=VAL :bar1
=VAL :FOO1
=VAL :BAR1
-MAP
=VAL :map2
+MAP
=VAL :foo2
=VAL :bar2
=VAL :FOO2
=VAL :BAR2
-MAP
-MAP
-DOC
-STR
)",
       PsTree::state st_map, st_mapmap;
       ps._begin_stream();     ___
       ps._begin_doc();     ___
       ps._begin_map_val_block(st_map);     ___
       ps._add_key_scalar_plain("map1");     ___
       ps._begin_map_val_block(st_mapmap);     ___
       ps._add_key_scalar_plain("foo1");     ___
       ps._add_val_scalar_plain("bar1");     ___
       ps._add_key_scalar_plain("FOO1");     ___
       ps._add_val_scalar_plain("BAR1");     ___
       ps._end_map();     ___
       ps._add_key_scalar_plain("map2");     ___
       ps._begin_map_val_block(st_mapmap);     ___
       ps._add_key_scalar_plain("foo2");     ___
       ps._add_val_scalar_plain("bar2");     ___
       ps._add_key_scalar_plain("FOO2");     ___
       ps._add_val_scalar_plain("BAR2");     ___
       ps._end_map();     ___
       ps._end_map();     ___
       ps._end_doc();     ___
       ps._end_stream();     ___
    )


//-----------------------------------------------------------------------------

PSTEST(SeqSeqFlow,
       "[[foo1,bar1,baz1],[foo2,bar2,baz2]]\n",
       R"(+STR
+DOC
+SEQ []
+SEQ []
=VAL :foo1
=VAL :bar1
=VAL :baz1
-SEQ
+SEQ []
=VAL :foo2
=VAL :bar2
=VAL :baz2
-SEQ
-SEQ
-DOC
-STR
)",
       PsTree::state st_seq, st_seqseq;
       ps._begin_stream();     ___
       ps._begin_doc();     ___
       ps._begin_seq_val_flow(st_seq);     ___
       ps._begin_seq_val_flow(st_seqseq);     ___
       ps._add_val_scalar_plain("foo1");     ___
       ps._add_val_scalar_plain("bar1");     ___
       ps._add_val_scalar_plain("baz1");     ___
       ps._end_seq();     ___
       ps._begin_seq_val_flow(st_seqseq);     ___
       ps._add_val_scalar_plain("foo2");     ___
       ps._add_val_scalar_plain("bar2");     ___
       ps._add_val_scalar_plain("baz2");     ___
       ps._end_seq();     ___
       ps._end_seq();     ___
       ps._end_doc();     ___
       ps._end_stream();     ___
    )


//-----------------------------------------------------------------------------

PSTEST(SeqSeqBlock,
       R"(
- - foo1
  - bar1
  - baz1
- - foo2
  - bar2
  - baz2
)",
       R"(+STR
+DOC
+SEQ
+SEQ
=VAL :foo1
=VAL :bar1
=VAL :baz1
-SEQ
+SEQ
=VAL :foo2
=VAL :bar2
=VAL :baz2
-SEQ
-SEQ
-DOC
-STR
)",
       PsTree::state st_seq, st_seqseq;
       ps._begin_stream();     ___
       ps._begin_doc();     ___
       ps._begin_seq_val_block(st_seq);     ___
       ps._begin_seq_val_block(st_seqseq);     ___
       ps._add_val_scalar_plain("foo1");     ___
       ps._add_val_scalar_plain("bar1");     ___
       ps._add_val_scalar_plain("baz1");     ___
       ps._end_seq();     ___
       ps._begin_seq_val_block(st_seqseq);     ___
       ps._add_val_scalar_plain("foo2");     ___
       ps._add_val_scalar_plain("bar2");     ___
       ps._add_val_scalar_plain("baz2");     ___
       ps._end_seq();     ___
       ps._end_seq();     ___
       ps._end_doc();     ___
       ps._end_stream();     ___
    )

} // namespace yml
} // namespace c4


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// this is needed to use the test case library

#ifndef RYML_SINGLE_HEADER
#include "c4/substr.hpp"
#endif

namespace c4 {
namespace yml {
struct Case;
Case const* get_case(csubstr /*name*/)
{
    return nullptr;
}
} // namespace yml
} // namespace c4
