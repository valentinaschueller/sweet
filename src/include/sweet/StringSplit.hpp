/*
 * StringSplit.hpp
 *
 *  Created on: 12 Feb 2017
 *      Author: Martin Schreiber <SchreiberX@gmail.com>
 */

#ifndef SRC_INCLUDE_SWEET_STRINGSPLIT_HPP_
#define SRC_INCLUDE_SWEET_STRINGSPLIT_HPP_

#include <vector>
#include <string>
#include <string>

#include "SWEETError.hpp"

class StringSplit
{
public:
	static
	std::vector<std::string> split(
			const std::string &i_string,
			const std::string &i_delimiter
	)
	{
		std::vector<std::string> ret;

		// create a copy of the string
		std::string s = i_string;

		// http://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
		std::size_t pos = 0;
		while ((pos = s.find(i_delimiter)) != std::string::npos) {
			std::string found_string = s.substr(0, pos);
		    ret.push_back(found_string);
		    s.erase(0, pos + i_delimiter.length());
		}

		ret.push_back(s);

		return ret;
	}

public:
	/**
	 * @brief fill a vector of doubles with a string whose substrings are separated by comma
	 * 
	 * If the string contains more elements than o_doubles, function throws an error.
	 * If the string contains less elements than o_doubles, o_double gets filled up with the 
	 * last value of the string.
	 * 
	 * @param i_str : the input string (example: "2,3,0.1")
	 * @param o_doubles : the output vector to be filled (example: here it should have >= 3 elements)
	 * @return int : the number of substrings contained in i_str (example: returns 3)
	 */
	static
	int split_n_doubles(
			const std::string & i_str,
			std::vector<double> & o_doubles
	)
	{
		std::vector<std::string> substrings = split(i_str, ",");
		size_t number_of_substrings = substrings.size();
		size_t number_of_doubles = o_doubles.size();
		double last_value{};

        if (number_of_substrings > number_of_doubles) 
        {
            SWEETError("More values given than expected in vector!");
		    return -1;
        }

        for (size_t iter = 0; iter < number_of_substrings; iter++)
        {
            o_doubles.at(iter) = atof(substrings.at(iter).c_str());
        }
		if (number_of_substrings < number_of_doubles)
		{
			last_value = o_doubles.at(number_of_substrings - 1);
			for (size_t iter = number_of_substrings; iter < number_of_doubles; iter++)
			{
				o_doubles.at(iter) = last_value;
			}
		}
        return number_of_substrings;
	}
};



#endif /* SRC_INCLUDE_SWEET_STRINGSPLIT_HPP_ */
