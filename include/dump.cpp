#include "dictionary.hpp"

namespace sshash {

void dictionary::dump(std::string const& filename) {
    uint64_t num_kmers = size();
    uint64_t num_minimizers = m_minimizers.size();
    uint64_t num_super_kmers = m_buckets.offsets.size();

    std::ofstream out(filename);

    std::cout << "dumping super-k-mers to file '" << filename << "'..." << std::endl;

    /* Write header and a dummy empty line "N". */
    out << '>' << m_k << ':' << m_m << ':' << num_kmers << ':' << num_minimizers << ':'
        << num_super_kmers << "\nN\n";

    for (uint64_t bucket_id = 0; bucket_id != num_minimizers; ++bucket_id) {
        auto [begin, end] = m_buckets.locate_bucket(bucket_id);
        for (uint64_t super_kmer_id = begin; super_kmer_id != end; ++super_kmer_id) {
            uint64_t offset = m_buckets.offsets.access(super_kmer_id);
            auto [_, contig_end] = m_buckets.offset_to_id(offset, m_k);
            bit_vector_iterator bv_it(m_buckets.strings, 2 * offset);
            uint64_t window_size = std::min<uint64_t>(m_k - m_m + 1, contig_end - offset - m_k + 1);
            uint64_t prev_minimizer = constants::invalid_uint64;
            bool super_kmer_header_written = false;
            for (uint64_t w = 0; w != window_size; ++w) {
                uint64_t kmer = bv_it.read_and_advance_by_two(2 * m_k);
                uint64_t minimizer = util::compute_minimizer(kmer, m_k, m_m, m_seed);
                if (m_canonical_parsing) {
                    uint64_t kmer_rc = util::compute_reverse_complement(kmer, m_k);
                    minimizer = std::min<uint64_t>(
                        minimizer, util::compute_minimizer(kmer_rc, m_k, m_m, m_seed));
                }
                if (!super_kmer_header_written) {
                    /* Write super-kmer header. */
                    out << '>' << bucket_id << ':' << super_kmer_id - begin << ':'
                        << util::uint64_to_string_no_reverse(minimizer, m_m) << '\n';
                    out << util::uint64_to_string_no_reverse(kmer, m_k);
                    super_kmer_header_written = true;
                } else {
                    if (minimizer != prev_minimizer) {
                        break;
                    } else {
                        out << util::uint64_to_char(kmer >> (2 * (m_k - 1)));
                    }
                }
                prev_minimizer = minimizer;
            }
            out << '\n';
        }
    }

    out.close();

    std::cout << "DONE" << std::endl;
}

}  // namespace sshash
