/*************************************************************************\
 * AdapterRemoval - cleaning next-generation sequencing reads            *
 *                                                                       *
 * Copyright (C) 2011 by Stinus Lindgreen - stinus@binf.ku.dk            *
 * Copyright (C) 2014 by Mikkel Schubert - mikkelsch@gmail.com           *
 *                                                                       *
 * If you use the program, please cite the paper:                        *
 * S. Lindgreen (2012): AdapterRemoval: Easy Cleaning of Next Generation *
 * Sequencing Reads, BMC Research Notes, 5:337                           *
 * http://www.biomedcentral.com/1756-0500/5/337/                         *
 *                                                                       *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 * This program is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. *
\*************************************************************************/
#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <memory>

#include "argparse.h"
#include "fastq.h"
#include "alignment.h"
#include "statistics.h"



struct alignment_info;


/**
 * Configuration store, containing all user-supplied options / default values,
 * as well as help-functions using these options.
 */
class userconfig
{
public:
	/**
	 * @param name Name of program.
	 * @param version Version string excluding program name.
	 * @param help Help text describing program.
	 */
    userconfig(const std::string& name,
           	   const std::string& version,
           	   const std::string& help);

    /** Parses a set of commandline arguments. */
    argparse::parse_result parse_args(int argc, char *argv[]);

    /** Returns new statistics object, initialized using usersettings. */
    std::auto_ptr<statistics> create_stats() const;


    enum alignment_type
    {
        //! Valid alignment according to user settings
        valid_alignment,
        //! Alignment with negative score
        poor_alignment,
        //! Read not aligned; too many mismatches, not enough bases, etc.
        not_aligned
    };

    /** Characterize an alignment based on user settings. */
    alignment_type evaluate_alignment(const alignment_info& alignment) const;

    /** Returns true if the alignment is sufficient for collapsing. */
    bool is_alignment_collapsible(const alignment_info& alignment) const;

    /** Returns true if the read matches the quality criteria set by the user. **/
    bool is_acceptable_read(const fastq& seq) const;


    std::auto_ptr<std::ostream> open_with_default_filename(
                                        const std::string& key,
                                        const std::string& postfix,
                                        bool compressed = true) const;


    /** Attempts to trim barcodes from a read, if this is enabled. */
    void trim_barcodes_if_enabled(fastq& read, statistics& stats) const;

    /** Trims a read if enabled, returning the #bases removed from each end. */
    fastq::ntrimmed trim_sequence_by_quality_if_enabled(fastq& read) const;


    //! Argument parser setup to parse the arguments expected by AR
    argparse::parser argparser;

    //! Prefix used for output files for which no filename was explicitly set
    std::string basename;
    //! Path to input file containing mate 1 reads (required)
    std::string input_file_1;
    //! Path to input file containing mate 2 reads (for PE reads)
    std::string input_file_2;

    //! Set to true if both --input1 and --input2 are set.
    bool paired_ended_mode;

    //! Pairs of adapters; may only contain the first value in SE enabled
    fastq_pair_vec adapters;

    //! Nucleotide barcodes to be trimmed from the 5' termini of mate 1 reads
    //! Only the first value in the pair is defined.
    fastq_pair_vec barcodes;

    //! The minimum length of trimmed reads (ie. genomic nts) to be retained
    unsigned min_genomic_length;
    //! The maximum length of trimmed reads (ie. genomic nts) to be retained
    unsigned max_genomic_length;
    //! The minimum required genomic overlap before collapsing reads into one.
    unsigned min_alignment_length;
    //! Rate of mismatches determining the threshold for a an acceptable
    //! alignment, depending on the length of the alignment. But see also the
    //! limits set in the function 'evaluate_alignment'.
    double mismatch_threshold;

    //! Quality format expected in input files.
    std::auto_ptr<fastq_encoding> quality_input_fmt;
    //! Quality format to use when writing FASTQ records.
    std::auto_ptr<fastq_encoding> quality_output_fmt;

    //! If true, read termini are trimmed for low-quality bases.
    bool trim_by_quality;
    //! The highest quality score which is considered low-quality
    unsigned low_quality_score;

    //! If true, ambiguous bases (N) at read termini are trimmed.
    bool trim_ambiguous_bases;
    //! The maximum number of ambiguous bases (N) in an read; reads exceeding
    //! this number following trimming (optionally) are discarded.
    unsigned max_ambiguous_bases;

    //! If true, PE reads overlapping at least 'min_alignment_length' are
    //! collapsed to generate a higher quality consensus sequence.
    bool collapse;
    // Allow for slipping basepairs by allowing missing bases in adapter
    unsigned shift;

    //! RNG seed for randomly selecting between to bases with the same quality
    //! when collapsing overllapping PE reads.
    unsigned seed;

    //! If true, the program attempts to identify the adapter pair of PE reads
    bool identify_adapters;

    //! If true, only error messages are printed to STDERR
    bool quiet;

    //! The maximum number of threads used by the program
    unsigned max_threads;

    //! GZip compression enabled / disabled
    bool gzip;
    //! GZip compression level used for output reads
    unsigned int gzip_level;

    //! BZip2 compression enabled / disabled
    bool bzip2;
    //! BZip2 compression level used for output reads
    unsigned int bzip2_level;

private:
    //! Not implemented
    userconfig(const userconfig&);
    //! Not implemented
    userconfig& operator=(const userconfig&);


    /** Sets up adapter sequences based on user settings.
     *
     * @return True on success, false otherwise.
     */
    bool setup_adapter_sequences();

    /** Sets up barcode (5prime) sequences based on user settings.
     *
     * @return True on success, false otherwise.
     */
    bool setup_barcode_sequences();


    /** Reads adapter sequences from a file.
     *
     * @param filename Path to text file containing adapter sequences
     * @param adapters Adapters or adapter pairs are appended to this list.
     * @param name Name of sequence type being read.
     * @param paired_ended For paired ended mode; expect pairs of sequences.
     * @return True on success, false otherwise.
     *
     * PCR1 adapters are expected to be found in column 1, while PCR2 adapters
     * are expected to be found in column 2 (if 'paried_ended' is true). These
     * are expected to contain only the standard nucleotides (ACGTN).
     **/
    bool read_adapter_sequences(const std::string& filename,
                                fastq_pair_vec& adapters,
                                const std::string& name,
                                bool paired_ended = true) const;

    //! Sink for --adapter1, adapter sequence expected at 3' of mate 1 reads
    std::string adapter_1;
    //! Sink for --adapter2, adapter sequence expected at 3' of mate 2 reads
    std::string adapter_2;
    //! Sink for --adapter-list; list of adapter #1 and #2 sequences
    std::string adapter_list;
    //! Sink for --5prime, barcode to be trimmed at 5' of mate 1 reads
    std::string barcode;
    //! Sink for --5prime-list; list of adapter sequences used for --5prime
    std::string barcode_list;

    //! Sink for user-supplied quality score formats; use quality_input_fmt.
    std::string quality_input_base;
    //! Sink for user-supplied quality score formats; use quality_output_fmt.
    std::string quality_output_base;
    //! Sink for maximum quality score for input / output
    unsigned quality_max;
};


#endif
