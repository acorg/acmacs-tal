#include <cctype>
#include <stack>

#include "acmacs-base/read-file.hh"
#include "acmacs-tal/newick.hh"

// https://en.wikipedia.org/wiki/Newick_format

// ----------------------------------------------------------------------

class TokenizerError : public std::runtime_error { public: using std::runtime_error::runtime_error; };

class Tokenizer
{
  public:
    enum TokenType { BeginSubtree, Leaf, EndSubtree, EndTree };

    struct Token
    {
        Token() : type{EndTree} {}
        Token(TokenType a_type) : type{a_type} {}
        Token(TokenType a_type, Token&& src) : type{a_type}, name(std::move(src.name)), edge_length(std::move(src.edge_length)) {}
        Token(std::string_view a_name, std::string_view a_edge) : type{EndTree}, name(a_name), edge_length(a_edge) {}

        TokenType type;
        std::string_view name{};
        std::string_view edge_length{};
    };

    Tokenizer(std::string_view data) : data_{data}, data_all_{data} {}

    Token next()
    {
        skip_ws();
        if (data_.empty())
            return Token{EndTree};
        switch (data_.front()) {
            case '(':
                data_.remove_prefix(1);
                return Token{BeginSubtree};
            case ')':
                data_.remove_prefix(1);
                return Token(EndSubtree, name_edge());
            case ';':
                data_.remove_prefix(1);
                return Token{EndTree};
            default:
                return Token{Leaf, name_edge()};
        }
        // throw TokenizerError("internal");
    }

    std::string_view rest() const { return data_; }

  private:
    std::string_view data_;
    std::string_view data_all_;

    Token name_edge()
    {
        if (data_.empty())
            return Token{};
        std::string_view name;
        if (data_.front() != ',' && data_.front() != ':')
            name = name_or_edge();
        switch (data_.front()) {
            case ',':
                data_.remove_prefix(1);
                return Token{name, std::string_view{}};
            case ')':
            case '(': // comma missing
            case ';':
                return Token{name, std::string_view{}};
            case ':': {
                data_.remove_prefix(1);
                const auto edge = name_or_edge();
                skip_ws();
                if (!data_.empty() && data_.front() == ',')
                    data_.remove_prefix(1);
                return Token{name, edge};
            }
            default:
                throw TokenizerError(fmt::format("unexpected symbol '{}' at pos {}", data_.front(), data_.data() - data_all_.data()));
        }
    }

    std::string_view name_or_edge()
    {
        const auto make_result = [this](size_t size) {
            const auto result = data_.substr(0, size);
            data_.remove_prefix(size);
            return result;
        };

        skip_ws();
        for (size_t pos = 0; pos < data_.size(); ++pos) {
            switch (data_[pos]) {
                case ':':
                case ',':
                case '(': // comma missing
                case ')':
                case ';':
                case ' ':
                case '\n':
                    return make_result(pos);
                default:
                    if (std::isspace(data_[pos]))
                        return make_result(pos);
                    break;
            }
        }
        throw TokenizerError("unexpected end of file");
    }

    void skip_ws()
    {
        while (!data_.empty() && std::isspace(data_.front()))
            data_.remove_prefix(1);
    }
};

// ----------------------------------------------------------------------

acmacs::tal::v3::Tree acmacs::tal::v3::newick_import(std::string_view filename)
{
    Tree tree;
    tree.data_buffer(acmacs::file::read(filename));
    Tokenizer tokenizer{tree.data_buffer()};
    try {
        std::stack<Node*> node_stack;
        for (auto token = tokenizer.next(); token.type != Tokenizer::EndTree; token = tokenizer.next()) {
            switch (token.type) {
                case Tokenizer::BeginSubtree:
                    if (node_stack.empty())
                        node_stack.push(&tree);
                    else
                        node_stack.push(&node_stack.top()->add_subtree());
                    break;
                case Tokenizer::Leaf:
                    // fmt::print(stderr, "DEBUG: Leaf {} :{} -- {}\n", token.name, token.edge_length, tokenizer.rest().substr(0, 20));
                    node_stack.top()->add_leaf(SeqId{token.name}, EdgeLength{token.edge_length});
                    break;
                case Tokenizer::EndSubtree:
                    node_stack.top()->seq_id = SeqId{token.name};
                    node_stack.top()->edge_length = EdgeLength{token.edge_length};
                    node_stack.pop();
                    break;
                case Tokenizer::EndTree:
                    break;
            }
        }
        if (!node_stack.empty())
            throw NewickImportError{"newick import error: unexpected end of data{}"};
    }
    catch (TokenizerError& err) {
        throw NewickImportError{fmt::format("newick import error: {}", err)};
    }
    return tree;

} // acmacs::tal::v3::newick_import

// ----------------------------------------------------------------------

static std::string export_node(const acmacs::tal::v3::Node& node);

std::string acmacs::tal::v3::newick_export(const Tree& tree)
{
    std::string result = export_node(tree);
    result.append(1, ';');
    return result;

} // acmacs::tal::v3::newick_export

// ----------------------------------------------------------------------

std::string export_node(const acmacs::tal::v3::Node& node)
{
    std::string result;
    if (!node.hidden) {
        if (!node.subtree.empty()) {
            result.append(1, '(');
            for (const auto& sub_node : node.subtree) {
                if (result.back() != '(')
                    result.append(1, ',');
                result.append(export_node(sub_node));
            }
            result.append(1, ')');
        }
        if (!node.seq_id.empty())
            result.append(node.seq_id);
        if (!node.edge_length.is_zero()) {
            result.append(1, ':');
            result.append(node.edge_length.as_string());
        }
    }
    return result;

} // export_node

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
