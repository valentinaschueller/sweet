/*
 * SPHMatrix.hpp
 *
 *  Created on: 24 Aug 2016
 *      Author: martin
 */

#ifndef SRC_INCLUDE_SPH_BANDEDMATRIX_HPP_
#define SRC_INCLUDE_SPH_BANDEDMATRIX_HPP_

#include <sweet/MemBlockAlloc.hpp>
#include "../sph/SphereDataConfig.hpp"



/**
 * Matrix to store coefficients related to Spherical Harmonics
 */
template <typename T>
class BandedMatrix
{
	/**
	 * Data stores the diagonal and off-diagonal components for a matrix which is to be inverted.
	 * This matrix is partitioned by independent chunks for each mode m.
	 *
	 * The off-diagonal components connect the l-modes only!
	 *
	 * EXAMPLE:
	 *
	 *   Multiplying the data matrix with a vector P would require P to be in the following format:
	 *   P is given by P_n^m
	 *
	 * The the Vector P is given by
	 *   P = (P_0^0, P_1^0, P_2^0, P_3^0, P_1^1, P_2^1, P_3^1, P_2^2, P_3^2, P_3^3)^T
	 */
public:
	T *data;

public:
	/**
	 * Data storage format for Fortran
	 */
	T *fortran_data;

public:
	int halosize_off_diagonal;
	int num_diagonals;

	SphereDataConfig *sphConfig;


public:
	BandedMatrix()	:
		data(nullptr),
		fortran_data(nullptr),
		halosize_off_diagonal(-1),
		num_diagonals(-1),
		sphConfig(nullptr)
	{
	}



	/**
	 * Zero all matrix coefficients
	 */
	void zeroAll()
	{
		for (std::size_t i = 0; i < sphConfig->spec_num_elems*num_diagonals; i++)
			data[i] = T(0);
	}



	/**
	 * Setup data storage
	 */
	void setup(
			SphereDataConfig *i_sphConfig,				///< Handler to sphConfig
			int i_halosize_offdiagonal = 0		///< Size of the halo around. A value of 2 allocates data for 5 diagonals.
	)
	{
		assert(data == nullptr);

		sphConfig = i_sphConfig;

		halosize_off_diagonal = i_halosize_offdiagonal;
		num_diagonals = 2*halosize_off_diagonal+1;

		data = MemBlockAlloc::alloc<T>(sizeof(T)*sphConfig->spec_num_elems*num_diagonals);

		zeroAll();
	}



	/**
	 * Return matrix row  which is related to the specified modes
	 */
	T *getMatrixRow(
			int n,		///< row related to P Legendre mode n
			int m		///< row related to P Fourier mode n
	)
	{
		std::size_t idx = sphConfig->getArrayIndexByModes(n, m);
		return data+idx*num_diagonals;
	}





	/**
	 * Return reference to an element in the row to the specified value
	 */
	const T &rowElement_getRef(
			T *io_row,		///< pointer to current row
			int i_row_n,	///< row related to P Legendre mode n
			int i_row_m,	///< row related to P Fourier mode n
			int rel_n		///< Relative Legendre mode n (e.g. -1 or +2)
	)
	{
		static T dummy = 0;

		if (i_row_n < i_row_m)
			return dummy;

//		assert(i_row_n >= i_row_m);
		assert(i_row_m >= 0);
		assert(i_row_m <= sphConfig->spec_m_max);

		int n = i_row_n+rel_n;

		if (n < 0 || n < i_row_m || n > sphConfig->spec_n_max)
			return dummy;

		int idx = rel_n + halosize_off_diagonal;

		assert(idx >= 0 && idx < num_diagonals);

		return io_row[idx];
	}



	/**
	 * Return reference to an element in the row to the specified value
	 */
	void rowElement_set(
			T *io_row,		///< pointer to current row
			int i_row_n,	///< row related to P Legendre mode n
			int i_row_m,	///< row related to P Fourier mode n
			int rel_n,		///< Relative Legendre mode n (e.g. -1 or +2)
			T i_value
	)
	{
		if (i_row_n < i_row_m)
			return;

//		assert(i_row_n >= i_row_m);
		assert(i_row_m >= 0);
		assert(i_row_m <= sphConfig->spec_m_max);

		int n = i_row_n+rel_n;

		if (n < 0 || n < i_row_m || n > sphConfig->spec_n_max)
			return;

		int idx = rel_n + halosize_off_diagonal;

		assert(idx >= 0 && idx < num_diagonals);

		io_row[idx] = i_value;
	}


	/**
	 * Return reference to an element in the row to the specified value
	 */
	void rowElement_add(
			T *io_row,		///< pointer to current row
			int i_row_n,	///< row related to P Legendre mode n
			int i_row_m,	///< row related to P Fourier mode n
			int rel_n,		///< Relative Legendre mode n (e.g. -1 or +2)
			const T &i_value
	)
	{
		if (i_row_n < i_row_m)
			return;

//		assert(i_row_n >= i_row_m);
		assert(i_row_m >= 0);
		assert(i_row_m <= sphConfig->spec_m_max);

		int n = i_row_n+rel_n;

		if (n < 0 || n < i_row_m || n > sphConfig->spec_n_max)
			return;

		int idx = rel_n + halosize_off_diagonal;

		assert(idx >= 0 && idx < num_diagonals);

		io_row[idx] += i_value;
	}


	void shutdown()
	{
		if (data != nullptr)
		{
			MemBlockAlloc::free(data, sizeof(T)*sphConfig->spec_num_elems*num_diagonals);
			data = nullptr;
		}

		if (fortran_data != nullptr)
		{
			MemBlockAlloc::free(fortran_data, sizeof(T)*sphConfig->spec_num_elems*num_diagonals);
			fortran_data = nullptr;
		}
	}



	~BandedMatrix()
	{
		shutdown();
	}



	void convertToFortranArray()
	{
		if (fortran_data == nullptr)
			fortran_data = MemBlockAlloc::alloc<T>(sizeof(T)*sphConfig->spec_num_elems*num_diagonals);

		for (int j = 0; j < sphConfig->spec_num_elems; j++)
			for (int i = 0; i < num_diagonals; i++)
				fortran_data[i*sphConfig->spec_num_elems + j] = data[j*num_diagonals + i];
	}



	void print()
	{
		std::size_t idx = 0;
		for (int m = 0; m <= sphConfig->spec_m_max; m++)
		{
			std::cout << "Meridional block M=" << m << " with N=[" << m << ", " << sphConfig->spec_n_max << "]" << std::endl;

			for (int n = m; n <= sphConfig->spec_n_max; n++)
			{
				if (n == m)
				{
					for (int hn = n-halosize_off_diagonal; hn < n+halosize_off_diagonal+1; hn++)
					{
						std::cout << hn;
						if (hn != n+halosize_off_diagonal)
							std::cout << "\t";
					}
					std::cout << std::endl;
					for (int hn = n-halosize_off_diagonal; hn < n+halosize_off_diagonal+1; hn++)
					{
						std::cout << "*******";
						if (hn != n+halosize_off_diagonal)
							std::cout << "\t";
					}
					std::cout << std::endl;
				}

				for (int i = 0; i < num_diagonals; i++)
				{
					std::cout << data[idx*num_diagonals+i];
					if (i != num_diagonals-1)
						std::cout << "\t";
				}
				std::cout << std::endl;

				idx++;
			}
		}
	}

	void print_mblock(int m)
	{
//		std::size_t idx = 0;
//		for (int m = 0; m <= sphConfig->spec_m_max; m++)
		{
			std::size_t idx = sphConfig->getArrayIndexByModes(m, m);
			std::cout << "Meridional block M=" << m << " with N=[" << m << ", " << sphConfig->spec_n_max << "]" << std::endl;

			for (int n = m; n <= sphConfig->spec_n_max; n++)
			{
				if (n == m)
				{
					for (int hn = n-halosize_off_diagonal; hn < n+halosize_off_diagonal+1; hn++)
					{
						std::cout << hn;
						if (hn != n+halosize_off_diagonal)
							std::cout << "\t";
					}
					std::cout << std::endl;
					for (int hn = n-halosize_off_diagonal; hn < n+halosize_off_diagonal+1; hn++)
					{
						std::cout << "*******";
						if (hn != n+halosize_off_diagonal)
							std::cout << "\t";
					}
					std::cout << std::endl;
				}

				for (int i = 0; i < num_diagonals; i++)
				{
					std::cout << data[idx*num_diagonals+i];
					if (i != num_diagonals-1)
						std::cout << "\t";
				}
				std::cout << std::endl;

				idx++;
			}
		}
	}



	void printFortran()
	{
		assert(fortran_data != nullptr);

		std::size_t idx = 0;
		for (int m = 0; m <= sphConfig->spec_m_max; m++)
		{
			std::cout << "Meridional block M=" << m << " with N=[" << m << ", " << sphConfig->spec_n_max << "]" << std::endl;

			for (int n = m; n <= sphConfig->spec_n_max; n++)
			{
				if (n == m)
				{
					for (int hn = n-halosize_off_diagonal; hn < n+halosize_off_diagonal+1; hn++)
					{
						std::cout << hn;
						if (hn != n+halosize_off_diagonal)
							std::cout << "\t";
					}

					std::cout << std::endl;
					for (int hn = n-halosize_off_diagonal; hn < n+halosize_off_diagonal+1; hn++)
					{
						std::cout << "*******";
						if (hn != n+halosize_off_diagonal)
							std::cout << "\t";
					}
					std::cout << std::endl;
				}

				for (int i = 0; i < num_diagonals; i++)
				{
					std::cout << fortran_data[idx+i*sphConfig->spec_num_elems];
					if (i != num_diagonals-1)
						std::cout << "\t";
				}
				std::cout << std::endl;

				idx++;
			}
		}
	}
};


#endif /* SRC_INCLUDE_SPH_BANDEDMATRIX_HPP_ */
