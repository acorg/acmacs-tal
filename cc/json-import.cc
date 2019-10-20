#include "acmacs-base/read-file.hh"
#include "acmacs-base/in-json-parser.hh"
#include "acmacs-tal/json-import.hh"

// ----------------------------------------------------------------------

namespace
{
    class node_data : public in_json::stack_entry
    {
      public:
        enum class array_processing { none, subnodes, hi_names, aa_substs, clades };

        node_data(acmacs::tal::v3::Node& node) : node_{node} {}

        const char* injson_name() override { return "node_data"; }

        void injson_put_real(std::string_view data) override
        {
            if (key_.size() == 1) {
                switch (key_.front()) {
                  case 'c':
                      node_.cumulative_edge_length = acmacs::tal::v3::EdgeLength{data};
                      break;
                  case 'l':
                      node_.edge_length = acmacs::tal::v3::EdgeLength{data};
                      break;
                  default:
                      throw in_json::parse_error(fmt::format("unsupported field: \"{}\"", key_));
                }
                reset_key();
            }
            else
                throw in_json::parse_error(fmt::format("unsupported field: \"{}\"", key_));
        }

        void injson_put_integer(std::string_view data) override
        {
            injson_put_real(data);
        }

        void injson_put_array() override
        {
            if (key_.size() == 1) {
                switch (key_.front()) {
                  case 't':
                      array_processing_ = array_processing::subnodes;
                      break;
                  case 'A':
                      array_processing_ = array_processing::aa_substs;
                      break;
                  case 'h':
                      array_processing_ = array_processing::hi_names;
                      break;
                  case 'L':
                      array_processing_ = array_processing::clades;
                      break;
                  default:
                      throw in_json::parse_error(fmt::format("unsupported field: \"{}\"", key_));
                }
            }
            else
                throw in_json::parse_error(fmt::format("unsupported field: \"{}\"", key_));
        }

        void injson_pop_array() override
        {
            array_processing_ = array_processing::none;
            reset_key();
        }

        std::unique_ptr<in_json::stack_entry> injson_put_object() override
        {
            switch (array_processing_) {
              case array_processing::subnodes:
                  return std::make_unique<node_data>(node_.subtree.emplace_back());
              case array_processing::aa_substs:
              case array_processing::hi_names:
              case array_processing::clades:
              case array_processing::none:
                  throw in_json::parse_error(fmt::format("unsupported object for key \"{}\"", key_));
            }
        }

        void injson_put_string(std::string_view data) override
        {
            switch (array_processing_) {
                case array_processing::none:
                    if (key_.size() == 1) {
                        switch (key_.front()) {
                            case 'n':
                                node_.seq_id = acmacs::tal::v3::SeqId{data};
                                break;
                            case 'a':
                                node_.aa_sequence = acmacs::seqdb::sequence_aligned_ref_t{data};
                                break;
                            case 'd':
                                node_.date = data;
                                break;
                            case 'C':
                                node_.continent = data;
                                break;
                            case 'D':
                                node_.country = data;
                                break;
                            default:
                                throw in_json::parse_error(fmt::format("unsupported string  \"{}\" for key \"{}\"", data, key_));
                        }
                    }
                    else
                        throw in_json::parse_error(fmt::format("unsupported string \"{}\" for key \"{}\"", data, key_));
                    reset_key();
                    break;
                case array_processing::hi_names:
                    node_.hi_names.push_back(data);
                    break;
                case array_processing::aa_substs:
                    node_.aa_substs.push_back(data);
                    break;
                case array_processing::clades:
                    node_.clades.push_back(data);
                    break;
                case array_processing::subnodes:
                    throw in_json::parse_error(fmt::format("unsupported string \"{}\" for key \"{}\"", data, key_));
            }
        }

        void injson_put_bool(bool val) override
        {
            if (key_.size() == 1) {
                switch (key_.front()) {
                  case 'H':
                      node_.hidden = val;
                      break;
                  default:
                      throw in_json::parse_error(fmt::format("unsupported field: \"{}\"", key_));
                }
            }
            else
                throw in_json::parse_error(fmt::format("unsupported field: \"{}\"", key_));
            reset_key();
        }

      private:
        acmacs::tal::v3::Node& node_;
        array_processing array_processing_{array_processing::none};
    };

    class tree_data : public in_json::stack_entry
    {
      public:
        tree_data(acmacs::tal::v3::Tree& tree) : tree_{tree} {}

        const char* injson_name() override { return "tree_data"; }

        std::unique_ptr<in_json::stack_entry> injson_put_object() override
        {
            if (key_ == "tree") {
                return std::make_unique<node_data>(tree_);
            }
            else
                throw in_json::parse_error(fmt::format("unsupported field: \"{}\"", key_));
        }

        void injson_put_string(std::string_view data) override
        {
            if (key_ == "  version") {
                if (data != "phylogenetic-tree-v3" && data != "newick-tree-v1")
                    throw in_json::parse_error(fmt::format("unsupported version: {}", data));
                reset_key();
            }
            else if (key_ == "  date" || key_ == "_")
                reset_key();
            else if (key_ == "v") {
                tree_.virus_type(data);
                reset_key();
            }
            else if (key_ == "l") {
                tree_.lineage(data);
                reset_key();
            }
            else
                throw in_json::parse_error(fmt::format("unsupported field: \"{}\"", key_));
        }

        void injson_put_array() override {}
        void injson_pop_array() override { reset_key(); }

      private:
        acmacs::tal::v3::Tree& tree_;
    };

    using sink = in_json::object_sink<acmacs::tal::v3::Tree, tree_data>;
}

// ----------------------------------------------------------------------

acmacs::tal::v3::Tree acmacs::tal::v3::json_import(std::string_view filename)
{
    Tree tree;
    tree.data_buffer(acmacs::file::read(filename));
    sink sink{tree};
    try {
        in_json::parse(sink, std::begin(tree.data_buffer()), std::end(tree.data_buffer()));
    }
    catch (in_json::parse_error& err) {
        throw JsonImportError{fmt::format("tree in json format import error: {}", err)};
    }
    tree.set_middle_node_names();
    return tree;

} // acmacs::tal::v3::json_import

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
