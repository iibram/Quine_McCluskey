#pragma once

#include <algorithm>
#include <iostream>
#include <sstream>
#include <format>
#include <vector>
#include <set>
#include <map>


// Min_Term types
constexpr uint8_t 	 DEFAULT = 0;					// `Min_Term` type = DEFAULT
constexpr uint8_t PRIME_IMPL = 1;					// `Min_Term` type = PRIME_IMPL
constexpr uint8_t  CORE_IMPL = 2;					// `Min_Term` type = CORE_IMPL

// Visuals
constexpr std::string_view X_MARK = "✅";		  // ✅ visualization for marked `Min_Term`s
constexpr std::string_view UNUSED = "⛔";		  // ⛔ visualization for unmarked `Min_Term`s (going to be a `PRIME_IMPL`)
constexpr std::string_view LEGEND_RED = "🟥";	   // 🟥 legend symbol RED (uncovered)
constexpr std::string_view LEGEND_GRN = "🟩";	   // 🟩 legend symbol GRN (covered)
constexpr std::string_view LEGEND_YLW = "🟨";	   // 🟨 legend symbol YLW (reason for being `CORE_IMPL`)

constexpr std::string_view RED = "\033[31m";		// sets the console color to RED
constexpr std::string_view GRN = "\033[32m";		// sets the console color to GREEN
constexpr std::string_view YLW = "\033[33m";		// sets the console color to YELLOW
constexpr std::string_view RST = "\033[0m";			// resets the console color to systems default

// QMC modes
constexpr uint8_t PHASE = 2;						// PHASE mode (space for each decimal = 2)
constexpr uint8_t TABLE = 0;						// TABLE mode (space for each decimal = 0)
constexpr bool FINAL_TABLE = true;


/**
 * @brief This class executes the entire minimization process using the Quine-McCluskey-Method (QMC) and outputs it
 * in a demonstrative manner — exactly as one would do on paper — for the term passed to it as an argument upon invocation.
 */
class QMC
{
public:
	QMC(std::string y, size_t y_length, int vars_size, std::string& id);
	~QMC() = default;

private:
	/**
	 * @brief Represents a (DNF) Min_Term during the Quine-McCluskey-Method (QMC)
	 */
	struct Min_Term
	{
		std::set<unsigned short> decimals;				// all decimal values this `Min_Term` represents (due to connections)
		std::string s_bits;								// the current binary representation of this `Min_Term` (going to be changed)
		std::string s_latex;							// the LaTeX code snippet for this `Min_Term`
		std::string s_vhdl;								// the VHDL code snippet for this `Min_Term`
		unsigned short type = DEFAULT;					// the determined type of this `Min_Term` (starts wtih `DEFAULT`)
		unsigned short span_beg = 0;					// the beginning decimal value of the covered span of this `Min_Term`
		unsigned short span_end = 0;					// the ending decimal value of the covered span of this `Min_Term`
		bool used = false;								// used status flag of this `Min_Term` while a QMC-Phase

		Min_Term(std::string str_bits);
		void insert_dec(unsigned short dec);
		void append_decimals(std::set<unsigned short> decs);
		std::string generate_sdecs(uint8_t mode) const;
		void set_str_reps(int b);

		bool operator<(const Min_Term& other) const;
	};

	/**
	 * @brief A helper struct for the coverage evaluation
	 */
	struct Info
	{
		std::set<Min_Term*> covering_primes;								// a set of all `PRIME_IMPL`s covering the specified `Min_Term`
		bool covered = false;												// is this `Min_Term` covered by a `CORE_IMPL` ?
	};

	std::stringstream ss;													// a `std::stringstream` for single console out calls
	std::vector<Min_Term> primes;											// `std::vector` of `Min_Term`s detected as primes
	std::map<unsigned short, std::vector<Min_Term>> curr_phase_map;			// [group number, `Min_Term`] mapping of the current phase
	std::map<unsigned short, std::vector<Min_Term>> next_phase_map;			// [group number, `Min_Term`] mapping of the next phase
	std::map<unsigned short, Info> spectrum;								// [dec of a `Min_Term`, `Info`] mapping (coverage detection)
	const int vars_size;													// the amount of terms (exp of the passed bit-string)
	unsigned short phase_num = 1;											// phase number counter

	int get_diff_idx(std::string sbits, std::string o_sbits) const;
	bool already_exists(const Min_Term& mt, const std::vector<Min_Term>& group);

	void process_phase();
	void compute_spectrum();
	void process_minimum_required_Min_Terms();
	//void print_implicant_table();
	void print_implicant_table(bool table_mode);
	void print_LaTeX_output();
	void print_VHDL_output(std::string& variable);

	void append_to_ss();
	void ss_out_n_reset();
};
