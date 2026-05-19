#include "core/math/CMatrix.h"
#include "core/math/CMatrix_ops.h"
#include <random>

CMatrix::CMatrix()
	:
	m_nRows(0),
	m_nCols(0)
{
}

CMatrix::CMatrix(std::size_t nRows, std::size_t nCols)
	:
	m_nRows(nRows),
	m_nCols(nCols)
{
	m_data.resize(nRows * nCols);
}

CMatrix::CMatrix(const CMatrix& other)
	:
	m_nRows(other.m_nRows),
	m_nCols(other.m_nCols)
{
	m_data = other.m_data;
}

CMatrix::CMatrix(CMatrix&& other)
	:
	m_nRows(other.m_nRows),
	m_nCols(other.m_nCols)
{
	m_data = std::move(other.m_data);

	other.m_nRows = 0;
	other.m_nCols = 0;
	other.m_data.clear();
}

CMatrix& CMatrix::operator=(const CMatrix& other)
{
	if (&other != this)
	{
		m_nRows = other.m_nRows;
		m_nCols = other.m_nCols;

		m_data = other.m_data;
	}

	return *this;
}

CMatrix& CMatrix::operator=(CMatrix&& other)
{
	if (&other != this)
	{
		m_nRows = other.m_nRows;
		m_nCols = other.m_nCols;

		m_data = std::move(other.m_data);

		other.m_nRows = 0;
		other.m_nCols = 0;
		other.m_data.clear();
	}

	return *this;
}

float& CMatrix::operator()(std::size_t i, std::size_t j)
{
	assert(i * m_nCols + j < m_data.size());
	return m_data[i * m_nCols + j];
}

const float& CMatrix::operator()(std::size_t i, std::size_t j) const
{
	assert(i * m_nCols + j < m_data.size());
	return m_data[i * m_nCols + j];
}

std::size_t CMatrix::GetRows() const
{
	return m_nRows;
}

std::size_t CMatrix::GetCols() const
{
	return m_nCols;
}

CMatrix& CMatrix::operator+=(const CMatrix& other)
{
	assert(m_nRows == other.m_nRows && m_nCols == other.m_nCols);

	for (std::size_t i = 0; i < m_nRows; i++)
	{
		for (std::size_t j = 0; j < m_nCols; j++)
		{
			m_data[i * m_nCols + j] += other(i, j);
		}
	}

	return *this;
}

CMatrix& CMatrix::operator-=(const CMatrix& other)
{
	assert(m_nRows == other.m_nRows && m_nCols == other.m_nCols);

	for (std::size_t i = 0; i < m_nRows; i++)
	{
		for (std::size_t j = 0; j < m_nCols; j++)
		{
			m_data[i * m_nCols + j] -= other(i, j);
		}
	}

	return *this;
}

CMatrix& CMatrix::operator*=(const CMatrix& other)
{
	*this = *this * other;
	return *this;
}

CMatrix& CMatrix::operator/=(const float value)
{
	for (std::size_t i = 0; i < m_nRows; i++)
	{
		for (std::size_t j = 0; j < m_nCols; j++)
		{
			m_data[i * m_nCols + j] /= value;
		}
	}

	return *this;
}

CMatrix CMatrix::Transpose() const
{
	CMatrix result(m_nCols, m_nRows);

	for (std::size_t i = 0; i < m_nRows; i++)
	{
		for (std::size_t j = 0; j < m_nCols; j++)
		{
			result(j, i) = (*this)(i, j);
		}
	}

	return result;
}

float CMatrix::GetMax() const
{
	assert(!m_data.empty());

	float nMaxValue = m_data[0];

	for (std::size_t i = 0; i < m_nRows; i++)
	{
		for (std::size_t j = 0; j < m_nCols; j++)
		{
			if (m_data[i * m_nCols + j] > nMaxValue)
				nMaxValue = m_data[i * m_nCols + j];
		}
	}

	return nMaxValue;
}

void CMatrix::Reinitialize(std::size_t nRows, std::size_t nCols)
{
	m_nRows = nRows;
	m_nCols = nCols;

	m_data.resize(nRows * nCols);
}

void CMatrix::InitializeRandom()
{
	std::mt19937 gen(std::random_device{}());
	std::uniform_real_distribution<float> dist(-0.1f, 0.1f);

	for (std::size_t i = 0; i < m_nRows; i++)
	{
		for (std::size_t j = 0; j < m_nCols; j++)
		{
			m_data[i * m_nCols + j] = dist(gen);
		}
	}
}