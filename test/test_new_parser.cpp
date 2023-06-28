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

PSTEST(Foo,
       "{foo: bar}",
        R"(+STR
+DOC
+MAP {}
=VAL :foo
=VAL :bar
-MAP
-DOC
-STR
)",
       ps._begin_stream();
       ps._begin_doc();
       ps._begin_map_val_flow();
       ps._add_key_scalar_plain("foo");
       ps._add_val_scalar_plain("bar");
       ps._end_map();
       ps._end_doc();
       ps._end_stream();
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
