#ifdef RYML_SINGLE_HEADER
#include "ryml_all.hpp"
#else
#include "c4/yml/new_parser.hpp"
#include "c4/yml/std/string.hpp"
#include "c4/yml/emit.hpp"
#include "c4/yml/detail/print.hpp"
#endif
#include <gtest/gtest.h>

namespace c4 {
namespace yml {

struct EventSink
{
    std::string result;
    void operator() (csubstr s) { result.append(s.str, s.len); }
    void operator() (char c) { result += c; }
};

using PsEvents = ParserSink<true, EventSink>;
using PsTree = ParserSink<false, EventSink>;

template<template<class> class Fn>
void test_new_parser_events(std::string const& expected)
{
    EventSink sink;
    PsEvents ps(&sink);
    Fn<PsEvents> fn;
    fn(ps);
    EXPECT_EQ(sink.result, expected);
}

template<template<class> class Fn>
void test_new_parser_wtree(std::string const& expected)
{
    Tree t = {};
    PsTree ps(&t);
    Fn<PsTree> fn;
    fn(ps);
    #ifdef RYML_DBG
    print_tree(t);
    #endif
    std::string actual = emitrs_yaml<std::string>(t);
    #ifdef RYML_DBG
    printf("~~~\n%s~~~\n", actual.c_str());
    #endif
    EXPECT_EQ(actual, expected);
}


//-----------------------------------------------------------------------------

#define PSTEST(name, yaml, events)              \
/* declare a function that will produce a       \
   sequence of events */                        \
template<class Ps>                              \
void name##_impl(Ps &ps);                       \
                                                \
template<class Ps>                              \
struct name                                     \
{                                               \
    void operator() (Ps &ps)                    \
    {                                           \
        name##_impl(ps);                        \
    }                                           \
};                                              \
                                                \
TEST(NewParser, name##_events)                  \
{                                               \
    SCOPED_TRACE(#name "_events");              \
    test_new_parser_events<name>(events);       \
}                                               \
TEST(NewParser, name##_wtree)                   \
{                                               \
    SCOPED_TRACE(#name "_tree");                \
    test_new_parser_wtree<name>(yaml);          \
}                                               \
                                                \
template<class Ps>                              \
void name##_impl(Ps &ps)


//-----------------------------------------------------------------------------

#ifndef RYML_DBG
#define ___(stmt) stmt
#else
#define ___(stmt)                                                       \
    do                                                                  \
    {                                                                   \
       stmt;                                                            \
       if(ps.is_wtree)                                                  \
       {                                                                \
           printf("%s:%d: parent.id=%zu curr.id=%zu  " #stmt "\n",      \
                  __FILE__, __LINE__, ps.m_parent->id, ps.m_curr.id);   \
       }                                                                \
    } while(0);
#endif


//-----------------------------------------------------------------------------

PSTEST(DocScalarPlain,
       "foo\n",
        R"(+STR
+DOC
=VAL :foo
-DOC
-STR
)")
{
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._add_val_scalar_plain("foo");)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}


//-----------------------------------------------------------------------------

PSTEST(DocScalarSQuoted,
       "'foo'\n",
        R"(+STR
+DOC
=VAL 'foo
-DOC
-STR
)")
{
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._add_val_scalar_squoted("foo");)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}


//-----------------------------------------------------------------------------

PSTEST(DocScalarDQuoted,
       "\"foo\"\n",
        R"(+STR
+DOC
=VAL "foo
-DOC
-STR
)")
{
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._add_val_scalar_dquoted("foo");)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}


//-----------------------------------------------------------------------------

PSTEST(DocScalarLiteral,
       "|-\n  foo\n",
        R"(+STR
+DOC
=VAL |foo
-DOC
-STR
)")
{
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._add_val_scalar_literal("foo");)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}


//-----------------------------------------------------------------------------

PSTEST(DocScalarFolded,
       ">-\n  foo\n",
        R"(+STR
+DOC
=VAL >foo
-DOC
-STR
)")
{
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._add_val_scalar_folded("foo");)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}


//-----------------------------------------------------------------------------

PSTEST(DocStream,
       "--- doc0\n--- 'doc1'\n--- \"doc2\"\n",
        R"(+STR
+DOC ---
=VAL :doc0
-DOC
+DOC ---
=VAL 'doc1
-DOC
+DOC ---
=VAL "doc2
-DOC
-STR
)")
{
    PsTree::state st_stream;
    ___(ps._begin_stream();)
    ___(ps._begin_doc_expl(st_stream);)
    ___(ps._add_val_scalar_plain("doc0");)
    ___(ps._end_doc_expl(st_stream);)
    ___(ps._begin_doc_expl(st_stream);)
    ___(ps._add_val_scalar_squoted("doc1");)
    ___(ps._end_doc_expl(st_stream);)
    ___(ps._begin_doc_expl(st_stream);)
    ___(ps._add_val_scalar_dquoted("doc2");)
    ___(ps._end_doc_expl(st_stream);)
    ___(ps._end_stream();)
}


//-----------------------------------------------------------------------------

PSTEST(DocStreamImplicitDocFirst,
       "--- doc0\n--- doc1\n--- doc2\n",
        R"(+STR
+DOC
=VAL :doc0
-DOC
+DOC ---
=VAL :doc1
-DOC
+DOC ---
=VAL :doc2
-DOC
-STR
)")
{
    PsTree::state st_stream;
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._add_val_scalar_plain("doc0");)
    ___(ps._end_doc();)
    ___(ps._begin_doc_expl(st_stream);)
    ___(ps._add_val_scalar_plain("doc1");)
    ___(ps._end_doc_expl(st_stream);)
    ___(ps._begin_doc_expl(st_stream);)
    ___(ps._add_val_scalar_plain("doc2");)
    ___(ps._end_doc_expl(st_stream);)
    ___(ps._end_stream();)
}


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
)")
{
    PsTree::state st_map;
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._begin_map_val_flow(st_map);)
    ___(ps._add_key_scalar_plain("foo");)
    ___(ps._add_val_scalar_plain("bar");)
    ___(ps._add_key_scalar_plain("foo2");)
    ___(ps._add_val_scalar_plain("bar2");)
    ___(ps._end_map(st_map);)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}


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
)")
{
    PsTree::state st_map;
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._begin_map_val_block(st_map);)
    ___(ps._add_key_scalar_plain("foo");)
    ___(ps._add_val_scalar_plain("bar");)
    ___(ps._add_key_scalar_plain("foo2");)
    ___(ps._add_val_scalar_plain("bar2");)
    ___(ps._add_key_scalar_plain("foo3");)
    ___(ps._add_val_scalar_plain("bar3");)
    ___(ps._end_map(st_map);)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}


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
)")
{
    PsTree::state st_seq;
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._begin_seq_val_flow(st_seq);)
    ___(ps._add_val_scalar_plain("foo");)
    ___(ps._add_val_scalar_plain("bar");)
    ___(ps._add_val_scalar_plain("baz");)
    ___(ps._end_map(st_seq);)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}


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
)")
{
    PsTree::state st_seq;
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._begin_seq_val_block(st_seq);)
    ___(ps._add_val_scalar_plain("foo");)
    ___(ps._add_val_scalar_plain("bar");)
    ___(ps._add_val_scalar_plain("baz");)
    ___(ps._end_map(st_seq);)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}


//-----------------------------------------------------------------------------

PSTEST(MapMapFlow,
       "{map1: {foo1: bar1,FOO1: BAR1},map2: {foo2: bar2,FOO2: BAR2}}",
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
)")
{
    PsTree::state st_map, st_mapmap;
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._begin_map_val_flow(st_map);)
    ___(ps._add_key_scalar_plain("map1");)
    ___(ps._begin_map_val_flow(st_mapmap);)
    ___(ps._add_key_scalar_plain("foo1");)
    ___(ps._add_val_scalar_plain("bar1");)
    ___(ps._add_key_scalar_plain("FOO1");)
    ___(ps._add_val_scalar_plain("BAR1");)
    ___(ps._end_map(st_map);)
    ___(ps._add_key_scalar_plain("map2");)
    ___(ps._begin_map_val_flow(st_mapmap);)
    ___(ps._add_key_scalar_plain("foo2");)
    ___(ps._add_val_scalar_plain("bar2");)
    ___(ps._add_key_scalar_plain("FOO2");)
    ___(ps._add_val_scalar_plain("BAR2");)
    ___(ps._end_map(st_map);)
    ___(ps._end_map(st_map);)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}


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
)")
{
    PsTree::state st_map, st_mapmap;
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._begin_map_val_block(st_map);)
    ___(ps._add_key_scalar_plain("map1");)
    ___(ps._begin_map_val_block(st_mapmap);)
    ___(ps._add_key_scalar_plain("foo1");)
    ___(ps._add_val_scalar_plain("bar1");)
    ___(ps._add_key_scalar_plain("FOO1");)
    ___(ps._add_val_scalar_plain("BAR1");)
    ___(ps._end_map(st_map);)
    ___(ps._add_key_scalar_plain("map2");)
    ___(ps._begin_map_val_block(st_mapmap);)
    ___(ps._add_key_scalar_plain("foo2");)
    ___(ps._add_val_scalar_plain("bar2");)
    ___(ps._add_key_scalar_plain("FOO2");)
    ___(ps._add_val_scalar_plain("BAR2");)
    ___(ps._end_map(st_map);)
    ___(ps._end_map(st_map);)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}


//-----------------------------------------------------------------------------

PSTEST(SeqSeqFlow,
       "[[foo1,bar1,baz1],[foo2,bar2,baz2]]",
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
)")
{
    PsTree::state st_seq, st_seqseq;
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._begin_seq_val_flow(st_seq);)
    ___(ps._begin_seq_val_flow(st_seqseq);)
    ___(ps._add_val_scalar_plain("foo1");)
    ___(ps._add_val_scalar_plain("bar1");)
    ___(ps._add_val_scalar_plain("baz1");)
    ___(ps._end_seq(st_seq);)
    ___(ps._begin_seq_val_flow(st_seqseq);)
    ___(ps._add_val_scalar_plain("foo2");)
    ___(ps._add_val_scalar_plain("bar2");)
    ___(ps._add_val_scalar_plain("baz2");)
    ___(ps._end_seq(st_seq);)
    ___(ps._end_seq(st_seq);)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}


//-----------------------------------------------------------------------------

PSTEST(SeqSeqBlock,
       R"(- - foo1
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
)")
{
    PsTree::state st_seq, st_seqseq;
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._begin_seq_val_block(st_seq);)
    ___(ps._begin_seq_val_block(st_seqseq);)
    ___(ps._add_val_scalar_plain("foo1");)
    ___(ps._add_val_scalar_plain("bar1");)
    ___(ps._add_val_scalar_plain("baz1");)
    ___(ps._end_seq(st_seq);)
    ___(ps._begin_seq_val_block(st_seqseq);)
    ___(ps._add_val_scalar_plain("foo2");)
    ___(ps._add_val_scalar_plain("bar2");)
    ___(ps._add_val_scalar_plain("baz2");)
    ___(ps._end_seq(st_seq);)
    ___(ps._end_seq(st_seq);)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}


//-----------------------------------------------------------------------------

PSTEST(AnchorDocVal,
       "&val a\n",
       R"(+STR
+DOC
=VAL &val :a
-DOC
-STR
)")
{
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._add_val_anchor("val");)
    ___(ps._add_val_scalar_plain("a");)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}


//-----------------------------------------------------------------------------

PSTEST(AnchorExplDocVal,
       "--- &val a\n",
       R"(+STR
+DOC ---
=VAL &val :a
-DOC
-STR
)")
{
    PsTree::state st_doc;
    ___(ps._begin_stream();)
    ___(ps._begin_doc_expl(st_doc);)
    ___(ps._add_val_anchor("val");)
    ___(ps._add_val_scalar_plain("a");)
    ___(ps._end_doc_expl(st_doc);)
    ___(ps._end_stream();)
}


//-----------------------------------------------------------------------------

PSTEST(AnchorSeq,
       "&seq\n- &val1 val1\n- &val2 val2\n",
       R"(+STR
+DOC
+SEQ &seq
=VAL &val1 :val1
=VAL &val2 :val2
-SEQ
-DOC
-STR
)")
{
    PsTree::state st_seq;
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._add_val_anchor("seq");)
    ___(ps._begin_seq_val_block(st_seq);)
    ___(ps._add_val_anchor("val1");)
    ___(ps._add_val_scalar_plain("val1");)
    ___(ps._add_val_anchor("val2");)
    ___(ps._add_val_scalar_plain("val2");)
    ___(ps._end_seq(st_seq);)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}

PSTEST(AnchorSeqWithRef,
       "&seq\n- &val1 val1\n- *val1\n",
       R"(+STR
+DOC
+SEQ &seq
=VAL &val1 :val1
=ALI *val1
-SEQ
-DOC
-STR
)")
{
    PsTree::state st_seq;
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._add_val_anchor("seq");)
    ___(ps._begin_seq_val_block(st_seq);)
    ___(ps._add_val_anchor("val1");)
    ___(ps._add_val_scalar_plain("val1");)
    ___(ps._add_val_ref("*val1");)
    ___(ps._end_seq(st_seq);)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}


//-----------------------------------------------------------------------------

PSTEST(AnchorMapBlock,
       "&map\n&key1 key1: &val1 val1\n&key2 key2: &val2 val2\n",
       R"(+STR
+DOC
+MAP &map
=VAL &key1 :key1
=VAL &val1 :val1
=VAL &key2 :key2
=VAL &val2 :val2
-MAP
-DOC
-STR
)")
{
    PsTree::state st_map;
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._add_val_anchor("map");)
    ___(ps._begin_map_val_block(st_map);)
    ___(ps._add_key_anchor("key1");)
    ___(ps._add_key_scalar_plain("key1");)
    ___(ps._add_val_anchor("val1");)
    ___(ps._add_val_scalar_plain("val1");)
    ___(ps._add_key_anchor("key2");)
    ___(ps._add_key_scalar_plain("key2");)
    ___(ps._add_val_anchor("val2");)
    ___(ps._add_val_scalar_plain("val2");)
    ___(ps._end_map(st_map);)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}

PSTEST(AnchorMapBlockWithRef,
       "&map\n&rkey1 key1: &rval1 val1\n*rkey1: *rval1\n",
       R"(+STR
+DOC
+MAP &map
=VAL &rkey1 :key1
=VAL &rval1 :val1
=ALI *rkey1
=ALI *rval1
-MAP
-DOC
-STR
)")
{
    PsTree::state st_map;
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._add_val_anchor("map");)
    ___(ps._begin_map_val_block(st_map);)
    ___(ps._add_key_anchor("rkey1");)
    ___(ps._add_key_scalar_plain("key1");)
    ___(ps._add_val_anchor("rval1");)
    ___(ps._add_val_scalar_plain("val1");)
    ___(ps._add_key_ref("*rkey1");)
    ___(ps._add_val_ref("*rval1");)
    ___(ps._end_map(st_map);)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}


//-----------------------------------------------------------------------------

PSTEST(AnchorMapFlow,
       "&map\n{&key1 key1: &val1 val1,&key2 key2: &val2 val2}",
       R"(+STR
+DOC
+MAP {} &map
=VAL &key1 :key1
=VAL &val1 :val1
=VAL &key2 :key2
=VAL &val2 :val2
-MAP
-DOC
-STR
)")
{
    PsTree::state st_map;
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._add_val_anchor("map");)
    ___(ps._begin_map_val_flow(st_map);)
    ___(ps._add_key_anchor("key1");)
    ___(ps._add_key_scalar_plain("key1");)
    ___(ps._add_val_anchor("val1");)
    ___(ps._add_val_scalar_plain("val1");)
    ___(ps._add_key_anchor("key2");)
    ___(ps._add_key_scalar_plain("key2");)
    ___(ps._add_val_anchor("val2");)
    ___(ps._add_val_scalar_plain("val2");)
    ___(ps._end_map(st_map);)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}

// WATCHOUT: see https://play.yaml.io/main/parser?input=Jm1hcAomcmtleTEgZm9vOiAmcnZhbDEgYmFyCipya2V5MSA6ICpydmFsMQ==
PSTEST(AnchorMapFlowWithRef,
       "&map\n{&rkey1 key1: &rval1 val1,*rkey1: *rval1}",
       R"(+STR
+DOC
+MAP {} &map
=VAL &rkey1 :key1
=VAL &rval1 :val1
=ALI *rkey1
=ALI *rval1
-MAP
-DOC
-STR
)")
{
    PsTree::state st_map;
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._add_val_anchor("map");)
    ___(ps._begin_map_val_flow(st_map);)
    ___(ps._add_key_anchor("rkey1");)
    ___(ps._add_key_scalar_plain("key1");)
    ___(ps._add_val_anchor("rval1");)
    ___(ps._add_val_scalar_plain("val1");)
    ___(ps._add_key_ref("*rkey1");)
    ___(ps._add_val_ref("*rval1");)
    ___(ps._end_map(st_map);)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}


//-----------------------------------------------------------------------------

PSTEST(AnchorMapMapBlock,
       "&map\n&mapkey map: &mapval\n  &key1 key1: &val1 val1\n  &key2 key2: &val2 val2\n",
       R"(+STR
+DOC
+MAP &map
=VAL &mapkey :map
+MAP &mapval
=VAL &key1 :key1
=VAL &val1 :val1
=VAL &key2 :key2
=VAL &val2 :val2
-MAP
-DOC
-STR
)")
{
    PsTree::state st_map, st_mapmap;
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._add_val_anchor("map");)
    ___(ps._begin_map_val_block(st_map);)
    ___(ps._add_key_anchor("mapkey");)
    ___(ps._add_key_scalar_plain("map");)
    ___(ps._add_val_anchor("mapval");)
    ___(ps._begin_map_val_block(st_mapmap);)
    ___(ps._add_key_anchor("key1");)
    ___(ps._add_key_scalar_plain("key1");)
    ___(ps._add_val_anchor("val1");)
    ___(ps._add_val_scalar_plain("val1");)
    ___(ps._add_key_anchor("key2");)
    ___(ps._add_key_scalar_plain("key2");)
    ___(ps._add_val_anchor("val2");)
    ___(ps._add_val_scalar_plain("val2");)
    ___(ps._end_map(st_map);)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}

PSTEST(AnchorMapMapFlow,
       "&map\n{&mapkey map: &mapval {&key1 key1: &val1 val1,&key2 key2: &val2 val2}}",
       R"(+STR
+DOC
+MAP {} &map
=VAL &mapkey :map
+MAP {} &mapval
=VAL &key1 :key1
=VAL &val1 :val1
=VAL &key2 :key2
=VAL &val2 :val2
-MAP
-DOC
-STR
)")
{
    PsTree::state st_map, st_mapmap;
    ___(ps._begin_stream();)
    ___(ps._begin_doc();)
    ___(ps._add_val_anchor("map");)
    ___(ps._begin_map_val_flow(st_map);)
    ___(ps._add_key_anchor("mapkey");)
    ___(ps._add_key_scalar_plain("map");)
    ___(ps._add_val_anchor("mapval");)
    ___(ps._begin_map_val_flow(st_mapmap);)
    ___(ps._add_key_anchor("key1");)
    ___(ps._add_key_scalar_plain("key1");)
    ___(ps._add_val_anchor("val1");)
    ___(ps._add_val_scalar_plain("val1");)
    ___(ps._add_key_anchor("key2");)
    ___(ps._add_key_scalar_plain("key2");)
    ___(ps._add_val_anchor("val2");)
    ___(ps._add_val_scalar_plain("val2");)
    ___(ps._end_map(st_map);)
    ___(ps._end_doc();)
    ___(ps._end_stream();)
}


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
