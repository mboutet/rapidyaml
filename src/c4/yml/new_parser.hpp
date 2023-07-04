#ifndef _C4_YML_PARSE_HPP_
#define _C4_YML_PARSE_HPP_

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

template<bool is_events, class Sink>
struct NewParser // : public ParserEvents<NewParser, NewParserStackState>
{
    static constexpr const bool is_wtree = !is_events;
    using state = NewParserStackState;
    using state_ref = state& C4_RESTRICT;
    using state_cref = state const& C4_RESTRICT;

    Tree *m_tree; // use only for wtree
    Sink *m_sink; // use only for events

    state const* C4_RESTRICT m_parent;
    state m_curr;
    NodeData m_evdata;

public:

    NewParser(Tree *tree) : m_tree(tree), m_sink()
    {
        if constexpr (is_wtree)
        {
            m_curr.id = m_tree->root_id();
            m_curr.data = m_tree->_p(m_curr.id);
            m_parent = &m_curr;
        }
    }

    NewParser(Sink *sink) : m_tree(), m_sink(sink)
    {
        if constexpr (is_events)
            m_curr.data = &m_evdata;
    }

public:

    void _send_(csubstr s) { (*m_sink)(s); }
    void _send_(char c) { (*m_sink)(c); }
    void _enable_(type_bits bits)
    {
        m_curr.data->m_type.type = static_cast<NodeType_e>(m_curr.data->m_type.type|bits);
    }

    void _send_key_props_()
    {
        if(m_curr.data->m_type.type & (KEYANCH|KEYREF))
        {
            _send_('&');
            _send_(m_curr.data->m_key.anchor);
        }
    }
    void _send_val_props_()
    {
        if(m_curr.data->m_type.type & (VALANCH|VALREF))
        {
            _send_('&');
            _send_(m_curr.data->m_val.anchor);
        }
    }

    void _send_key_scalar_(csubstr s, char type)
    {
        _send_("=VAL ");
        _send_key_props_();
        _send_(type);
        _send_(s);
        _send_('\n');
    }

    void _send_val_scalar_(csubstr s, char type)
    {
        _send_("=VAL ");
        _send_val_props_();
        _send_(type);
        _send_(s);
        _send_('\n');
    }

public:

    void _add_()
    {
        m_curr.id = m_tree->append_child(m_parent->id);
        m_curr.data = m_tree->_p(m_curr.id);
    }
    void _add_(state_ref next_parent)
    {
        next_parent = m_curr;
        m_parent = &next_parent;
        m_curr.id = m_tree->append_child(m_parent->id);
        m_curr.data = m_tree->_p(m_curr.id);
    }
    void _end_(state_cref prev_parent)
    {
        m_parent = &prev_parent;
        const size_t last = m_tree->last_child(m_parent->id);
        if(m_curr.id != last)
            m_tree->move(m_curr.id, m_parent->id, last);
    }

public:

    void _begin_stream()
    {
        if constexpr (is_events)
            _send_("+STR\n");
    }
    void _end_stream()
    {
        if constexpr (is_events)
            _send_("-STR\n");
        else
            m_tree->remove(m_curr.id);
    }

    void _begin_doc()
    {
        if constexpr (is_events)
            _send_("+DOC\n");
        else
            _enable_(DOC);
    }
    void _end_doc()
    {
        if constexpr (is_events)
            _send_("-DOC\n");
    }

    void _begin_doc_expl()
    {
        if constexpr (is_events)
            _send_("+DOC ---\n");
        else
            _enable_(DOC);
    }
    void _end_doc_expl()
    {
        if constexpr (is_events)
            _send_("-DOC\n");
    }

public:

    void _begin_map_key_flow(state_ref)
    {
        if constexpr (is_events)
        {
            _send_("+MAP {}");
            _send_key_props_();
            _send_('\n');
        }
        else
        {
            C4_NOT_IMPLEMENTED();
        }
    }
    void _begin_map_key_block(state_ref)
    {
        if constexpr (is_events)
        {
            _send_("+MAP");
            _send_key_props_();
            _send_('\n');
        }
        else
        {
            C4_NOT_IMPLEMENTED();
        }
    }
    void _begin_map_val_flow(state_ref st)
    {
        if constexpr (is_events)
        {
            _send_("+MAP {}");
            _send_val_props_();
            _send_('\n');
        }
        else
        {
            _enable_(MAP|_WIP_STYLE_FLOW_SL);
            _add_(st);
        }
    }
    void _begin_map_val_block(state_ref st)
    {
        if constexpr (is_events)
        {
            _send_("+MAP");
            _send_val_props_();
            _send_('\n');
        }
        else
        {
            _enable_(MAP|_WIP_STYLE_BLOCK);
            _add_(st);
        }
    }
    void _end_map(state_cref prev_parent)
    {
        if constexpr (is_events)
            _send_("-MAP\n");
        else
            _end_(prev_parent);
    }

public:

    void _begin_seq_key_flow()
    {
        if constexpr (is_events)
        {
            _send_("+SEQ []");
            _send_key_props_();
            _send_('\n');
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
            _send_("+SEQ");
            _send_key_props_();
            _send_('\n');
        }
        else
        {
            C4_NOT_IMPLEMENTED();
        }
    }
    void _begin_seq_val_flow(state_ref next_parent)
    {
        if constexpr (is_events)
        {
            _send_("+SEQ []");
            _send_val_props_();
            _send_('\n');
        }
        else
        {
            _enable_(SEQ|_WIP_STYLE_FLOW_SL);
            _add_(next_parent);
        }
    }
    void _begin_seq_val_block(state_ref next_parent)
    {
        if constexpr (is_events)
        {
            _send_("+SEQ");
            _send_val_props_();
            _send_('\n');
        }
        else
        {
            _enable_(SEQ|_WIP_STYLE_BLOCK);
            _add_(next_parent);
        }
    }
    void _end_seq(state_cref prev_parent)
    {
        if constexpr (is_events)
            _send_("-SEQ\n");
        else
            _end_(prev_parent);
    }

public:

    void _add_key_scalar_plain(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _send_key_scalar_(unfiltered, ':');
        }
        else
        {
            m_curr.data->m_key.scalar = unfiltered;
            _enable_(KEY|_WIP_KEY_PLAIN);
        }
    }
    void _add_val_scalar_plain(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _send_val_scalar_(unfiltered, ':');
        }
        else
        {
            m_curr.data->m_val.scalar = unfiltered;
            _enable_(VAL|_WIP_VAL_PLAIN);
            _add_();
        }
    }

    void _add_key_scalar_dquoted(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _send_key_scalar_(unfiltered, '"');
        }
        else
        {
            m_curr.data->m_key.scalar = unfiltered;
            _enable_(KEY|_WIP_KEY_DQUO);
        }
    }
    void _add_val_scalar_dquoted(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _send_val_scalar_(unfiltered, '"');
        }
        else
        {
            m_curr.data->m_val.scalar = unfiltered;
            _enable_(VAL|_WIP_VAL_DQUO);
            _add_();
        }
    }

    void _add_key_scalar_squoted(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _send_key_scalar_(unfiltered, '\'');
        }
        else
        {
            m_curr.data->m_key.scalar = unfiltered;
            _enable_(KEY|_WIP_KEY_SQUO);
        }
    }
    void _add_val_scalar_squoted(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _send_val_scalar_(unfiltered, '\'');
        }
        else
        {
            m_curr.data->m_val.scalar = unfiltered;
            _enable_(VAL|_WIP_VAL_SQUO);
            _add_();
        }
    }

    void _add_key_scalar_literal(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _send_key_scalar_(unfiltered, '|');
        }
        else
        {
            m_curr.data->m_key.scalar = unfiltered;
            _enable_(KEY|_WIP_KEY_LITERAL);
        }
    }
    void _add_val_scalar_literal(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _send_val_scalar_(unfiltered, '|');
        }
        else
        {
            m_curr.data->m_val.scalar = unfiltered;
            _enable_(VAL|_WIP_VAL_LITERAL);
            _add_();
        }
    }

    void _add_key_scalar_folded(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _send_key_scalar_(unfiltered, '>');
        }
        else
        {
            m_curr.data->m_key.scalar = unfiltered;
            _enable_(VAL|_WIP_KEY_FOLDED);
        }
    }
    void _add_val_scalar_folded(csubstr unfiltered)
    {
        if constexpr (is_events)
        {
            _send_val_scalar_(unfiltered, '>');
        }
        else
        {
            m_curr.data->m_val.scalar = unfiltered;
            _enable_(VAL|_WIP_VAL_FOLDED);
            _add_();
        }
    }

public:

    void _add_key_tag(csubstr tag)
    {
        _enable_(KEYTAG);
        m_curr.data->m_key.tag = tag;
    }
    void _add_val_tag(csubstr tag)
    {
        _enable_(VALTAG);
        m_curr.data->m_val.tag = tag;
    }

    void _add_key_anchor(csubstr anchor)
    {
        _enable_(KEYANCH);
        m_curr.data->m_key.anchor = anchor;
    }
    void _add_val_anchor(csubstr anchor)
    {
        _enable_(VALANCH);
        m_curr.data->m_val.anchor = anchor;
    }

    void _add_key_ref(csubstr ref)
    {
        _enable_(KEYREF);
        m_curr.data->m_key.anchor = ref;
    }
    void _add_val_ref(csubstr ref)
    {
        _enable_(VALREF);
        m_curr.data->m_val.anchor = ref;
    }
};

} // namespace yml
} // namespace c4

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif /* _C4_YML_PARSE_HPP_ */
