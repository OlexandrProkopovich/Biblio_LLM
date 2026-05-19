#pragma once
#include <vector>
#include <cassert>

class CMatrix
{
public:
	using Storage = std::vector<float>;

	CMatrix();

	CMatrix(std::size_t nRows, std::size_t nCols);
	CMatrix(const CMatrix& other);
	CMatrix(CMatrix&& other);

	CMatrix& operator=(const CMatrix& other);
	CMatrix& operator=(CMatrix&& other);

	~CMatrix() = default;

	float& operator()(std::size_t i, std::size_t j);
	const float& operator()(std::size_t i, std::size_t j) const;

	std::size_t GetRows() const;
	std::size_t GetCols() const;

	CMatrix& operator+=(const CMatrix& other);
	CMatrix& operator-=(const CMatrix& other);
	CMatrix& operator*=(const CMatrix& other);
	CMatrix& operator/=(const float value);

	CMatrix Transpose() const;

	float GetMax() const;

	void Reinitialize(std::size_t nRows, std::size_t nCols);
	void InitializeRandom();

private:
	Storage m_data;

	std::size_t m_nRows;
	std::size_t m_nCols;
};