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
       "{foo: bar, foo2: bar2}",
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
       "[foo, bar, baz]",
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
