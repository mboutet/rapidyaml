#ifndef _C4_YML_PARSER_SINK_HPP_
#define _C4_YML_PARSER_SINK_HPP_

#ifndef _C4_YML_TREE_HPP_
#include "c4/yml/tree.hpp"
#endif

namespace c4 {
namespace yml {

struct NewParserStackState
{
    size_t id;
    NodeData* C4_RESTRICT data;
};

// while arriving at a suitable interface for the parser sink, we will
// use a single class for both implementations. Then it should be
// split into a sink for creating the tree and another to emit the
// event strings.
template<bool is_events, class Sink>
struct ParserSink;

using ParserSinkTree = ParserSink<false, bool>;
template<class Sink> using ParserSinkEvents = ParserSink<true, Sink>;


template<bool is_events, class EventStrSink>
struct ParserSink // : public ParserEvents<NewParser, NewParserStackState>
{
    static constexpr const bool is_wtree = !is_events;

    using state = NewParserStackState;
    using state_mut_ptr = state* C4_RESTRICT;
    using state_cref = state const& C4_RESTRICT;

    Tree *m_tree; // use only for wtree
    EventStrSink *m_sink; // use only for events

    state const* C4_RESTRICT m_parent;
    state m_currnode;
    NodeData m_evdata;

public:

    ParserSink() : m_tree(), m_sink(), m_parent(), m_currnode(), m_evdata() {}

    ParserSink(Tree *tree) : m_tree(tree), m_sink(), m_parent(), m_currnode(), m_evdata()
    {
        if constexpr (is_wtree)
        {
            m_currnode.id = m_tree->root_id();
            m_currnode.data = m_tree->_p(m_currnode.id);
            m_parent = &m_currnode;
        }
    }

    ParserSink(EventStrSink *sink) : m_tree(), m_sink(sink), m_parent(), m_currnode(), m_evdata()
    {
        if constexpr (is_events)
            m_currnode.data = &m_evdata;
    }

public:

    C4_ALWAYS_INLINE void _ev_send_(csubstr s) { (*m_sink)(s); }
    C4_ALWAYS_INLINE void _ev_send_(char c) { (*m_sink)(c); }

    void _ev_send_key_props_()
    {
        if(m_currnode.data->m_type.type & (KEYANCH|KEYREF))
        {
            _ev_send_(" &");
            _ev_send_(m_currnode.data->m_key.anchor);
        }
    }
    void _ev_send_val_props_()
    {
        if(m_currnode.data->m_type.type & (VALANCH|VALREF))
        {
            _ev_send_(" &");
            _ev_send_(m_currnode.data->m_val.anchor);
        }
    }

    void _ev_send_key_scalar_(csubstr s, char type)
    {
        _ev_send_("=VAL");
        _ev_send_key_props_();
        _ev_send_(' ');
        _ev_send_(type);
        _ev_send_(s);
        _ev_send_('\n');
    }
    void _ev_send_val_scalar_(csubstr s, char type)
    {
        _ev_send_("=VAL");
        _ev_send_val_props_();
        _ev_send_(' ');
        _ev_send_(type);
        _ev_send_(s);
        _ev_send_('\n');
    }

public:

    /** add another node to the current parent and set it as the current node */
    C4_ALWAYS_INLINE void _tr_add_() noexcept
    {
        m_currnode.id = m_tree->_append_child__unprotected(m_parent->id);
        m_currnode.data = m_tree->_p(m_currnode.id);
    }

    /** push a new parent, and add a child to the  */
    void _tr_add_(state_mut_ptr next_parent)
    {
        *next_parent = m_currnode; // save the current node to pop it later
        m_parent = next_parent; // push it
        _tr_add_(); // add the child on the next parent
    }
    void _tr_end_(state_cref prev_parent)
    {
        m_parent = &prev_parent;
        const size_t last = m_tree->last_child(m_parent->id);
        if(m_currnode.id != last)
            m_tree->move(m_currnode.id, m_parent->id, last);
    }

    C4_ALWAYS_INLINE void _tr_enable_(type_bits bits) noexcept
    {
        m_currnode.data->m_type.type = static_cast<NodeType_e>(m_currnode.data->m_type.type|bits);
    }

public:

    void _begin_stream()
    {
        if constexpr (is_events)
            _ev_send_("+STR\n");
    }
    void _end_stream()
    {
        if constexpr (is_events)
            _ev_send_("-STR\n");
        else
            m_tree->remove(m_currnode.id);
    }

    void _begin_doc()
    {
        if constexpr (is_events)
            _ev_send_("+DOC\n");
        else
            _tr_enable_(DOC);
    }
    void _end_doc()
    {
        if constexpr (is_events)
            _ev_send_("-DOC\n");
    }

    void _begin_doc_expl(state_mut_ptr next_parent)
    {
        if constexpr (is_events)
        {
            _ev_send_("+DOC ---\n");
        }
        else
        {
            if(m_tree->is_root(m_currnode.id))
            {
                _tr_enable_(STREAM);
                _tr_add_(next_parent);
            }
            else if(!m_tree->is_stream(m_tree->root_id()))
            {
                m_tree->remove(m_currnode.id);
                m_tree->set_root_as_stream();
                m_currnode.id = m_tree->root_id();
                m_currnode.data = m_tree->_p(m_currnode.id);
                _tr_add_(next_parent);
            }
            _tr_enable_(DOC);
        }
    }
    void _end_doc_expl(state_cref prev_parent)
    {
        if constexpr (is_events)
            _ev_send_("-DOC\n");
        else
        {
            _tr_end_(prev_parent);
        }
    }

public:

    void _begin_map_key_flow(state_mut_ptr)
    {
        if constexpr (is_events)
        {
            _ev_send_("+MAP {}");
            _ev_send_key_props_();
            _ev_send_('\n');
        }
        else
        {
            C4_NOT_IMPLEMENTED();
        }
    }
    void _begin_map_key_block(state_mut_ptr)
    {
        if constexpr (is_events)
        {
            _ev_send_("+MAP");
            _ev_send_key_props_();
            _ev_send_('\n');
        }
        else
        {
            C4_NOT_IMPLEMENTED();
        }
    }
    void _begin_map_val_flow(state_mut_ptr next_parent)
    {
        if constexpr (is_events)
        {
            _ev_send_("+MAP {}");
            _ev_send_val_props_();
            _ev_send_('\n');
        }
        else
        {
            _tr_enable_(MAP|_WIP_STYLE_FLOW_SL);
            _tr_add_(next_parent);
        }
    }
    void _begin_map_val_block(state_mut_ptr next_parent)
    {
        if constexpr (is_events)
        {
            _ev_send_("+MAP");
            _ev_send_val_props_();
            _ev_send_('\n');
        }
        else
        {
            _tr_enable_(MAP|_WIP_STYLE_BLOCK);
            _tr_add_(next_parent);
        }
    }
    void _end_map(state_cref prev_parent)
    {
        if constexpr (is_events)
            _ev_send_("-MAP\n");
        else
            _tr_end_(prev_parent);
    }

public:

    void _begin_seq_key_flow()
    {
        if constexpr (is_events)
        {
            _ev_send_("+SEQ []");
            _ev_send_key_props_();
            _ev_send_('\n');
        }
        else
        {
            C4_NOT_IMPLEMENTED();
        }
    }
    void _begin_seq_key_block()
    {
        if constexpr (is_events)
        {
            _ev_send_("+SEQ");
            _ev_send_key_props_();
            _ev_send_('\n');
        }
        else
        {
            C4_NOT_IMPLEMENTED();
        }
    }
    void _begin_seq_val_flow(state_mut_ptr next_parent)
    {
        if constexpr (is_events)
        {
            _ev_send_("+SEQ []");
            _ev_send_val_props_();
            _ev_send_('\n');
        }
        else
        {
            _tr_enable_(SEQ|_WIP_STYLE_FLOW_SL);
            _tr_add_(next_parent);
        }
    }
    void _begin_seq_val_block(state_mut_ptr next_parent)
    {
        if constexpr (is_events)
        {
            _ev_send_("+SEQ");
            _ev_send_val_props_();
            _ev_send_('\n');
        }
        else
        {
            _tr_enable_(SEQ|_WIP_STYLE_BLOCK);
            _tr_add_(next_parent);
        }
    }
    void _end_seq(state_cref prev_parent)
    {
        if constexpr (is_events)
            _ev_send_("-SEQ\n");
        else
            _tr_end_(prev_parent);
    }

public:

    void _add_key_scalar_plain(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _ev_send_key_scalar_(unfiltered, ':');
        }
        else
        {
            m_currnode.data->m_key.scalar = unfiltered;
            _tr_enable_(KEY|_WIP_KEY_PLAIN);
        }
    }
    void _add_val_scalar_plain(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _ev_send_val_scalar_(unfiltered, ':');
        }
        else
        {
            m_currnode.data->m_val.scalar = unfiltered;
            _tr_enable_(VAL|_WIP_VAL_PLAIN);
            _tr_add_();
        }
    }

    void _add_key_scalar_dquoted(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _ev_send_key_scalar_(unfiltered, '"');
        }
        else
        {
            m_currnode.data->m_key.scalar = unfiltered;
            _tr_enable_(KEY|_WIP_KEY_DQUO);
        }
    }
    void _add_val_scalar_dquoted(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _ev_send_val_scalar_(unfiltered, '"');
        }
        else
        {
            m_currnode.data->m_val.scalar = unfiltered;
            _tr_enable_(VAL|_WIP_VAL_DQUO);
            _tr_add_();
        }
    }

    void _add_key_scalar_squoted(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _ev_send_key_scalar_(unfiltered, '\'');
        }
        else
        {
            m_currnode.data->m_key.scalar = unfiltered;
            _tr_enable_(KEY|_WIP_KEY_SQUO);
        }
    }
    void _add_val_scalar_squoted(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _ev_send_val_scalar_(unfiltered, '\'');
        }
        else
        {
            m_currnode.data->m_val.scalar = unfiltered;
            _tr_enable_(VAL|_WIP_VAL_SQUO);
            _tr_add_();
        }
    }

    void _add_key_scalar_literal(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _ev_send_key_scalar_(unfiltered, '|');
        }
        else
        {
            m_currnode.data->m_key.scalar = unfiltered;
            _tr_enable_(KEY|_WIP_KEY_LITERAL);
        }
    }
    void _add_val_scalar_literal(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _ev_send_val_scalar_(unfiltered, '|');
        }
        else
        {
            m_currnode.data->m_val.scalar = unfiltered;
            _tr_enable_(VAL|_WIP_VAL_LITERAL);
            _tr_add_();
        }
    }

    void _add_key_scalar_folded(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _ev_send_key_scalar_(unfiltered, '>');
        }
        else
        {
            m_currnode.data->m_key.scalar = unfiltered;
            _tr_enable_(VAL|_WIP_KEY_FOLDED);
        }
    }
    void _add_val_scalar_folded(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _ev_send_val_scalar_(unfiltered, '>');
        }
        else
        {
            m_currnode.data->m_val.scalar = unfiltered;
            _tr_enable_(VAL|_WIP_VAL_FOLDED);
            _tr_add_();
        }
    }

public:

    void _add_key_tag(csubstr tag)
    {
        _tr_enable_(KEYTAG);
        m_currnode.data->m_key.tag = tag;
    }
    void _add_val_tag(csubstr tag)
    {
        _tr_enable_(VALTAG);
        m_currnode.data->m_val.tag = tag;
    }

    void _add_key_anchor(csubstr anchor)
    {
        _tr_enable_(KEYANCH);
        m_currnode.data->m_key.anchor = anchor;
    }
    void _add_val_anchor(csubstr anchor)
    {
        _tr_enable_(VALANCH);
        m_currnode.data->m_val.anchor = anchor;
    }

    void _add_key_ref(csubstr ref)
    {
        RYML_ASSERT(ref.begins_with('*'));
        if constexpr (is_events)
        {
            _ev_send_("=ALI ");
            _ev_send_(ref);
            _ev_send_('\n');
        }
        else
        {
            _tr_enable_(KEY|KEYREF);
            m_currnode.data->m_key.anchor = ref.sub(1);
            m_currnode.data->m_key.scalar = ref;
        }
    }
    void _add_val_ref(csubstr ref)
    {
        RYML_ASSERT(ref.begins_with('*'));
        if constexpr (is_events)
        {
            _ev_send_("=ALI ");
            _ev_send_(ref);
            _ev_send_('\n');
        }
        else
        {
            _tr_enable_(VAL|VALREF);
            m_currnode.data->m_val.anchor = ref.sub(1);
            m_currnode.data->m_val.scalar = ref;
            _tr_add_();
        }
    }
};

} // namespace yml
} // namespace c4

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif /* _C4_YML_PARSER_SINK_HPP_ */
