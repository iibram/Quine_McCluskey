#include "QMC.h"
#include <cstring>
#include <cmath>

/*
Examples: The (H) at the end of the y bitstring signals, that a heuristical evaluation must be performed to solve the minimization

8 Bit				16 Bit						32 Bit
y = "10011001"		y = "1001011010110101" (H)	y = "10101010101010101111000011110000"
y = "01101010"		y = "0110100011110001"		y = "11110000000011111111000000001111"
y = "11100010"		y = "1111000011110000"		y = "10011100111100001001110011110000" (H)


HARD ONES:
0101001111001111 (H)					1011110000110101 (H)
1  3  6  7  8  9  12  13  14  15		0  2  3  4  5  10  11  13  15
2  2  1  2  1  2   2   2   2   2		2  2  1  2  2   1   2   2   2
	  x     x								  x         x

CHAMPIONS LEAGUE:
0111110101101110 (H)
1  2  3  4  5  7  9  10  12  13  14
2  2  2  1  3  1  1   2   2   2   2
		 X     X  X
*/

/**
 * @brief $ ./main <y-string to minimize> <ID of the y-string for VHDL | leaving empty sets a default ID>
 */
int main(int argc, char* argv [])
{
	std::string y, id = "sig_y";

	if (argc == 2 || argc == 3)
	{
		y = argv[1];
		if (argc == 3 && (std::strlen(argv[2]) > 0)) id = argv[2];

		size_t y_length = y.length();
		double exp = std::log2(y_length);

		if (std::floor(exp) == std::ceil(exp))
		{
			int vars_size = static_cast<int>(exp);
			QMC qmc(y, y_length, vars_size, id);
		}

		else
		{
			std::cout << "Length of y-string is NOT a power of two !" << "\n";
			return -1;
		}
	}
	else
	{
		std::cout << "A y-string was NOT passed !" << "\n";
		return -1;
	}

	std::cout << "\nPress enter to quit . . .";
	std::cin.get();

	return 0;
}
