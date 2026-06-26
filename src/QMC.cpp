#include "QMC.h"


// ===========================================================================================================================================
// 																M i n _ T e r m
// ===========================================================================================================================================

/**
 * @brief Custom constructor.
 * @param str_bits the current binary representation of this Min_Term
 */
QMC::Min_Term::Min_Term(std::string& str_bits) : s_bits(str_bits) {}

/**
 * @brief Inserts the given (single) decimal to the `std::set` decimals of this `Min_Term`.
 * @param dec the decimal value which is to insert
 */
void QMC::Min_Term::insert_dec(unsigned short dec) { decimals.insert(dec); }

/**
 * @brief Tries to insert each decimal of the given `std::set` to the `std::set` of decimals of this `Min_Term`.
 * @param decs a `std::set` of decimals
 */
void QMC::Min_Term::append_decimals(std::set<unsigned short> decs)
{
	decimals.insert(decs.begin(), decs.end());
}

/**
 * @brief Generates a `std::string` of the decimal values this `Min_Term` represents.
 * @param mode `TABLE`=0 or `PHASE`=2. (Values also used to format the specific texts)
 * @return a `std::string` of the decimal values this `Min_Term` represents
 */
std::string QMC::Min_Term::generate_sdecs(uint8_t mode) const
{
	std::string sdecs = "";
	unsigned short i = 0;

	for (unsigned short dec : decimals)
	{
		if (i < 4)
			sdecs += (i++ == 0 ? std::format("{:{}}", dec, mode) : std::format(",{:{}}", dec, mode));

		else
		{
			sdecs += ",.";
			break;
		}
	}

	return sdecs;
}

/**
 * @brief Generates and sets the `LaTeX` and the `VHDL` code  representations of this `Min_Term` for additional usage.
 * @param vsize `QMC::vars_size` goes in. The amount of variables (exp of the y-string length)
 */
void QMC::Min_Term::set_str_reps(int vsize)
{
	bool first = true;
	s_vhdl = "(";
	for (char c : s_bits)
	{
		if (c == '-') { vsize--; continue; }

		std::string latex_x = "\\mathrm{x}_" + std::to_string(vsize);
		std::string vhdl_x = "x" + std::to_string(vsize--);
		s_latex += (c == '0' ? "\\overline{" + latex_x + "}" : latex_x);

		if (!first) s_vhdl += " and ";
		first = false;

		s_vhdl += (c == '0' ? "not " + vhdl_x : vhdl_x);
	}
	s_vhdl += ")";
}

/**
 * @brief CUSTOM COMPARATOR: Overloads the `<` operator for enabling customized sorting in a `std::set` of `Min_Term`s.
 * @param other another `Min_Term`
 * @return `bool`
 */
bool QMC::Min_Term::operator<(const Min_Term& other) const
{
	if (span_beg != other.span_beg)
		return span_beg < other.span_beg;
	return span_end < other.span_end;
}


// ===========================================================================================================================================
// 																	Q M C
// ===========================================================================================================================================

/**
 * @brief Custom constructor: Starts immediately to build the essential base group by the passed (pre checked) parameters and proceeds through
 * the Quine-McCluskey tabular method.
 *
 * @param y the output bit stream of the underlaying logic table which is to minimize
 * @param y_length the length of that output string (is a power of 2)
 * @param vars_size pre calculated size of the variables (exponent of the y_length)
 * @param id the VHDL signal id of y (default: "sig_y")
 */
QMC::QMC(std::string y, size_t y_length, int vars_size, std::string& id) : vars_size(vars_size)
{
	ss << std::string(11, ' ') << "Minimization of\n";
	ss << std::string((37 - y_length - 4) / 2, ' ') << "y = " << y << "\n\n";

	// init base (GRP mapped) Min_Terms
	for (unsigned int d = 0; d < y_length; d++)
	{
		if (y.at(d) == '1')
		{
			unsigned short grp = std::popcount((size_t)d);
			unsigned short dec = d;
			std::string sbits = "";

			for (int b = vars_size - 1; b >= 0; b--)
				sbits += (d >> b) & 0x1 ? '1' : '0';

			Min_Term mt = Min_Term(sbits);
			mt.insert_dec(d);
			curr_phase_map[grp].push_back({ mt });
		}
	}

	// guiding through the app
	process_phase();
	compute_spectrum();
	process_minimum_required_Min_Terms();
	print_implicant_table(FINAL_TABLE);
	print_LaTeX_output();
	print_VHDL_output(id);
}

// ---------------------------------------------------------- ESSENTIAL FUNCTIONS ------------------------------------------------------------

/**
 * @brief Builds recursively all the groups of any phase of the Quine-McCluskey Tabular Method (QMC), detects prime implicants and feeds the
 * `std::stringsstream` with relative informations. After all recursions has terminated, a single console out is called to output the entire
 * phase tables at once.
 */
void QMC::process_phase()
{
	next_phase_map.clear();

	// builing the groups of each phase
	for (auto& [key, val] : curr_phase_map)
	{
		unsigned short next_grp = key + 1;
		if (curr_phase_map.contains(next_grp))
		{
			for (auto& mt1 : val)
			{
				for (auto& mt2 : curr_phase_map[next_grp])
				{
					std::string sbits = mt1.s_bits;

					int diff_idx = get_diff_idx(sbits, mt2.s_bits);
					if (diff_idx != -1)
					{
						sbits.at(diff_idx) = '-';
						Min_Term mt = Min_Term(sbits);

						mt.append_decimals(mt1.decimals);
						mt.append_decimals(mt2.decimals);

						mt1.used = true;
						mt2.used = true;

						if (already_exists(mt, next_phase_map[key]))
							continue;

						next_phase_map[key].push_back({ mt });
					}
				}
			}
		}

		// adding PRIME_IMPLs to primes (the unchecked Terms per phase)
		for (auto& mt : val)
		{
			if (!mt.used)
			{
				mt.type = PRIME_IMPL;
				mt.span_beg = *(mt.decimals.begin());
				mt.span_end = *(mt.decimals.rbegin());
				mt.set_str_reps(vars_size);

				primes.push_back(mt);
			}
		}
	}

	// controling the recursion
	if (!next_phase_map.empty())
	{
		append_to_ss();

		phase_num += 1;
		curr_phase_map = std::move(next_phase_map);

		process_phase();
	}

	else // output all at once
	{
		append_to_ss();
		ss << "\n";
		ss_out_n_reset();
	}
}

/**
 * @brief Computes the spectrum (covered span) of any `PRIME_IMPL`, inserts these covered `Min_Term`s also as values to the mapped
 * `Info` struct. If any entry of the spectrum counts a single `Min_Term` it covers, a `CORE_IMPL` is detected and is marked as one.
 * Also the unfinished intermediate "Implicants Table" is output to the console, only when heuristics are to process.
 */
void QMC::compute_spectrum()
{
	bool heuristic_needed = false;

	// Phase 1: Build up the spectrum
	// each appearance of the decimal value of a Min_Term (covered by a PRIME_IMPL) is becoming
	// a KEY of the spectrum and the covering PRIME_IMPL is inserted to the set (Info)
	for (auto& prime : primes)
	{
		for (auto dec : prime.decimals)
			spectrum[dec].covering_primes.insert(&prime);
	}

	// Phase 2: Detect all CORE_IMPLs and mark their Min_Terms as clearly covered
	for (auto const& [key, info] : spectrum)
	{
		if (info.covering_primes.size() == 1)
		{
			Min_Term* core = *info.covering_primes.begin();
			core->type = CORE_IMPL;

			for (auto dec : core->decimals)
				spectrum[dec].covered = true;
		}
	}

	// Phase 3: Check whether there is still any uncovered minterm at all!
	for (auto const& [key, info] : spectrum)
	{
		if (!info.covered)
		{
			heuristic_needed = true;
			break;
		}
	}

	if (heuristic_needed)
		print_implicant_table(!FINAL_TABLE);
}

/**
 * @brief The elimination algorithm (NP-Problem): Is finding and processing for the uncovered `Min_Term`s the very best fit of a coverage.
 * If there is just a single uncovered `Min_Term` found, the best fit would be the minimum span of a `Min_Term` to cover this one, too.
 * If there is more than one uncovered `Min_Term`s found, a NP-Problem is to solve.
 *
 * In a candidate system (each round) it is selected the `PRIME_IMPL` with the widest span to cover the highest number of uncovered
 * `Min_Term`s. If a single leading candidate is found, the competition ends for that round. To break a tie situation, the `PRIME_IMPL`
 * with the minimum span is now winning the competition for this round. However, the winner is marked as a `CORE_IMPL`. The function
 * terminates when there is no uncovered `Min_Term`s left.
 */
void QMC::process_minimum_required_Min_Terms()
{
	while (true)
	{
		std::vector<unsigned short> uncovered_keys;

		for (auto const& [key, info] : spectrum)
			if (!info.covered) uncovered_keys.push_back(key);

		// elimination ends (if all Min Terms are covered by CORE_IMPLs)
		if (uncovered_keys.empty()) break;

		Min_Term* winner = nullptr;

		auto span = [](Min_Term* mt) { return mt->span_end - mt->span_beg; };

		// easy elimination
		if (uncovered_keys.size() == 1)
		{
			auto key = uncovered_keys.front();
			winner = *spectrum[key].covering_primes.begin();

			// selecting the Min_Term with the minimum span
			if (spectrum[key].covering_primes.size() > 1)
			{
				for (auto mt_ptr : spectrum[key].covering_primes)
					if (span(mt_ptr) < span(winner))
						winner = mt_ptr;
			}
		}

		else // uncovered > 1 (worst case)
		{
			std::map<Min_Term*, unsigned short> candidate_count;

			// collecting the candidates
			for (auto key : uncovered_keys)
				for (auto mt_ptr : spectrum[key].covering_primes)
					candidate_count[mt_ptr]++;

			// searching for the maximum span
			unsigned short max_cover = 0;
			std::vector<Min_Term*> top_candidates;
			for (auto const& [mt_ptr, count] : candidate_count)
			{
				if (count > max_cover)
				{
					max_cover = count;
					top_candidates.clear();
					top_candidates.push_back(mt_ptr);
				}
				else if (count == max_cover)
					top_candidates.push_back(mt_ptr);
			}

			winner = top_candidates.front();

			// breaking the tie by the minimal span
			for (auto mt_ptr : top_candidates)
				if (span(mt_ptr) < span(winner))
					winner = mt_ptr;
		}

		// marking the winner as CORE_IMPL and the covered keys as true
		winner->type = CORE_IMPL;
		for (auto dec : winner->decimals)
			spectrum[dec].covered = true;
	}
}

// ----------------------------------------------------------- Output Functions --------------------------------------------------------------

/**
 * @brief Prints the colorful "Implicants Table" of the Quine-McCluskey Tabular Method (QMC) to the console.
 * @param table_mode `FINAL_TABLE` or NOT.
 */
void QMC::print_implicant_table(bool table_mode)
{
	uint8_t LHS_WIDTH = 16;
	uint8_t RHS_WIDTH = spectrum.size() << 2;		// spectrum.size() = number of decimals covered by the PRIME IMPLs (4 = COL_WIDTH)
	uint8_t RHS_HALF = (RHS_WIDTH >> 1) - 1;		// for specific centering
	uint8_t COL_WIDTH = 4;							// space for a decimal. (_1__ / _10_ / _100) and a (_X__) alligns with the leading number
	bool safe = (spectrum.size() > 3);

	if (table_mode == FINAL_TABLE)
		ss << std::format("{0:=^{1}} Implicants Table (FINAL) {0:=^{1}}\n", "", (safe ? RHS_HALF - 3 : 3));
	else
		ss << std::format("{0:=^{1}} Implicants (Pre-Heuristic) {0:=^{1}}\n", "", (safe ? RHS_HALF - 4 : 3));

	ss << std::format("{:^{}}| ", "Primes", LHS_WIDTH);

	// all decimals the CORE_IMPLs cover (GRN: is clearly covered, RED: must be evaluated heuristically)
	for (auto const& [key, info] : spectrum)
	{
		if (info.covered) ss << GRN; else ss << RED;
		ss << std::format("{:<{}}", std::format(" {}", key), COL_WIDTH);
	}

	ss << RST << std::format("\n{:-^{}}+{:-^{}}-", "", LHS_WIDTH, "", RHS_WIDTH);

	for (auto& prime : primes)
	{
		std::string_view COLOR = (prime.type == CORE_IMPL ? GRN : RED); 	// (GRN: essential, RED: redundant / unresovled)
		ss << COLOR;
		ss << "\n" << std::format("{:>15}", std::format("[{}]", prime.generate_sdecs(TABLE)));
		ss << RST << " | " << COLOR;

		bool started = false;
		bool ended = false;
		bool single = false;

		for (auto const& [key, info] : spectrum)
		{
			single = (info.covering_primes.size() == 1);
			ss << COLOR;

			if (prime.decimals.contains(key))								// prime covers the decimal ?
			{
				if (key == prime.span_end)									// span ends
				{
					ss << (started ? "-" : " ");
					if (single) ss << YLW;
					ss << "X";
					ended = true;
				}
				else if (key == prime.span_beg)								// span begins
				{
					if (single) ss << YLW << " X" << COLOR << "--";
					else ss << " X--";
					started = true;
				}
				else if (single)
					ss << "-" << YLW << "X" << COLOR << "--";

				else
					ss << "-X--";
			}
			else if (!ended)												// when doesn't cover
				ss << (started ? "----" : "    ");

			ss << RST;
		}
	}
	ss << std::format("\n{:-^{}}+{:-^{}}-\n", "", LHS_WIDTH, "", RHS_WIDTH);
	if (table_mode == FINAL_TABLE)
		ss << LEGEND_GRN << " Essential  " << LEGEND_RED << " Redundant/Unresolved  " << LEGEND_YLW << " Core Trigger\n\n";

	ss_out_n_reset();
}

/**
 * @brief "Parses" the calculated minimized function and prints it as a LaTeX instruction to the console.
 */
void QMC::print_LaTeX_output()
{
	ss << "========== Generated LaTeX Source Code ==========\n$f(";

	int b = vars_size;
	while (b > 0)
	{
		ss << "x_" << b-- << ",";
		if (b == 0) ss << "\b)=\n";
	}

	bool first_term = true;
	for (auto const& prime : primes)
	{
		if (prime.type != CORE_IMPL) continue;

		if (!first_term) ss << "\n\\lor";
		first_term = false;

		ss << prime.s_latex;
	}
	ss << "$\n";
	ss_out_n_reset();
}

/**
 * @brief "Parses" the calculated minimized function and prints it as a VHDL instruction to the console. Although the minimized function
 * should work properly, in hardware design it is not recommended to drop such big function at once. For an 8 bit wide small logic output
 * function y passed to this app, it could be a good solution.
 *
 * @param variable the identifier of the logical signal (`std_logic`) inside the VHDL design
 */
void QMC::print_VHDL_output(std::string& variable)
{
	const unsigned short indent = (unsigned short)variable.length() + 4;

	ss << "============== Generated VHDL Logic =============\n";
	ss << variable << " <= ";

	bool first_term = true;
	for (auto const& prime : primes)
	{
		if (prime.type != CORE_IMPL) continue;

		std::string term = prime.s_vhdl;

		if (!first_term)
			ss << "\n" << std::string(indent, ' ') << "or ";
		else
			first_term = false;

		ss << term;
	}
	ss << ";\n";
	ss_out_n_reset();
}

// ------------------------------------------------------------ Helper Functions -------------------------------------------------------------

/**
 * @brief Compares two given string representations of logic terms (e.g. "1001") from left to right and returns either the index of a detected
 * single different "bit"; or -1, when there is no difference or more than one.
 *
 * @param sbits1 the "bit" string of a logical term
 * @param sbits2 the "bit" string of another logical term
 * @return the index of the single different "bit", or -1 if there is no difference or more than one
 */
int QMC::get_diff_idx(std::string sbits1, std::string sbits2) const
{
	int diff_idx = -1;

	for (int i = 0; i < vars_size; i++)
	{
		if (sbits1.at(i) != sbits2.at(i))
		{
			if (diff_idx == -1)
				diff_idx = i;
			else
				return -1;
		}
	}

	return diff_idx;
}

/**
 * @brief Checks and returns if the given `Min_Term` already exists in the given group.
 * @param mt the `Min_Term` to check
 * @param group the group to check
 * @return `bool`
 */
bool QMC::already_exists(const Min_Term& mt, const std::vector<Min_Term>& group)
{
	return std::any_of(group.begin(), group.end(), [&](const Min_Term& existing)
	{ return existing.decimals == mt.decimals; });
}

/**
 * @brief Appends the next phase table to the `std::stringstream`.
 */
void QMC::append_to_ss()
{
	std::string border = std::string(14, '=');
	ss << border << " PHASE " << phase_num << " " << border << "\n";

	for (auto const& [key, val] : curr_phase_map)
	{
		bool first = true;
		for (auto const& mt : val)
		{
			ss << std::format("{:16}{:13}", mt.generate_sdecs(PHASE), mt.s_bits);
			ss << std::format("{:>4}", (mt.used ? X_MARK : UNUSED));
			ss << (first ? std::format("{:>4}\n", key) : "\n");
			first = false;
		}
		border = std::string(37, '-');
		ss << border << "\n";
	}
}

/**
 * @brief Outputs the buffer of the `std::stringstream`, resets its buffer and clears its flags for a clean reusage.
 */
void QMC::ss_out_n_reset()
{
	std::cout << ss.str() << "\n";
	ss.str("");
	ss.clear();
}
