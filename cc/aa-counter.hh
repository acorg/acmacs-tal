#pragma once

#include <array>
#include <algorithm>
#include <numeric>

#include "acmacs-base/fmt.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class AACounter
    {
      public:
        constexpr static const size_t number_of_positions{1300};
        constexpr static const size_t number_of_aa{17};
        constexpr static const char nothing{'.'}; // dot is to ease reporting

        using count_t = uint32_t;

        struct value_type
        {
            char aa{nothing};
            count_t count{0};

            bool operator<(const value_type& rhs) const { return count < rhs.count; }

            void format_to(fmt::memory_buffer& out, std::string_view format, double total) const
            {
                fmt::format_to(out, format, fmt::arg("value", aa), fmt::arg("counter", count), fmt::arg("counter_percent", static_cast<double>(count) / total * 100.0));
            }
        };

        using data_type = std::array<value_type, number_of_positions * number_of_aa>;
        using iterator = typename data_type::iterator;
        using const_iterator = typename data_type::const_iterator;
        using pos_t = size_t;

        AACounter() noexcept = default;

        bool empty(pos_t pos) const { return at(pos)->aa == nothing; }

        void count(pos_t pos, char aa, count_t increment = 1)
        {
            for (auto iter = at(pos); iter != at(pos + 1); ++iter) {
                if (iter->aa == aa) {
                    iter->count += increment;
                    return;
                }
                else if (iter->aa == nothing) {
                    iter->aa = aa;
                    iter->count = increment;
                    return;
                }
            }
            throw std::runtime_error{AD_FORMAT("AACounter::count: increase number_of_aa in aa-counter.hh:17")};
        }

        void add(pos_t pos, const AACounter& other)
        {

            if (auto obeg = other.at(pos); obeg->aa != nothing) {
                if (const auto beg = at(pos); beg->aa == nothing)
                    std::copy(obeg, other.at(pos + 1), beg);
                else {
                    for (const auto oend = other.at(pos + 1); obeg != oend && obeg->aa != nothing; ++obeg)
                        count(pos, obeg->aa, obeg->count);
                }
            }
        }

        void add(const AACounter& other)
            {
                for (pos_t pos{0}; pos < number_of_positions; ++pos)
                    add(pos, other);
            }

        const_iterator at(pos_t pos) const { return std::next(std::cbegin(data_), static_cast<ssize_t>(pos * number_of_aa)); }
        iterator at(pos_t pos) { return std::next(std::begin(data_), static_cast<ssize_t>(pos * number_of_aa)); }

        char max(pos_t pos) const { return std::max_element(at(pos), at(pos + 1))->aa; }
        const value_type& max_count(pos_t pos) const { return *std::max_element(at(pos), at(pos + 1)); }
        auto total(pos_t pos) const
        {
            return std::accumulate(at(pos), at(pos + 1), count_t{0}, [](count_t sum, const auto& val) { return sum + val.count; });
        }

        std::string report_sorted_max_first(pos_t pos, std::string_view format) const
            {
                std::vector<value_type> pairs;
                size_t total{0};
                for (auto iter = at(pos); iter != at(pos + 1) && iter->aa != nothing; ++iter) {
                    pairs.push_back(*iter);
                    total += iter->count;
                }
                std::sort(std::begin(pairs), std::end(pairs), [](const auto& e1, const auto& e2) { return e1.count > e2.count; });

                fmt::memory_buffer out;
                for (const auto& en : pairs)
                    en.format_to(out, format, static_cast<double>(total));
                return fmt::to_string(out);
            }

      private:
        data_type data_;
    };
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
