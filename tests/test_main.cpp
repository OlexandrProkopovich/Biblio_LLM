#include <gtest/gtest.h>
#include "core/math/CMatrix.h"
#include "core/math/CMatrix_ops.h"
#include "core/math/CActivations.h"
#include "core/tokenizer/CTokenizer.h"
#include "core/model/CEmbedding.h"
#include "core/model/CAttention.h"
#include "core/model/CFFLayer.h"
#include "core/model/CTransformerBlock.h"

TEST(CMatrixTest, DefaultConstructor)
{
    CMatrix m;

    SUCCEED();
}

TEST(CMatrixTest, SizeConstructorAndAccess)
{
    CMatrix m(3, 4);

    m(0, 0) = 1.0f;
    m(2, 3) = 5.0f;

    EXPECT_FLOAT_EQ(m(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(m(2, 3), 5.0f);
}

TEST(CMatrixTest, WriteAndReadAllElements)
{
    CMatrix m(3, 3);

    for (size_t i = 0; i < 3; ++i)
        for (size_t j = 0; j < 3; ++j)
            m(i, j) = static_cast<float>(i * 10 + j);

    for (size_t i = 0; i < 3; ++i)
        for (size_t j = 0; j < 3; ++j)
            EXPECT_FLOAT_EQ(m(i, j), static_cast<float>(i * 10 + j));
}

TEST(CMatrixTest, CopyConstructor)
{
    CMatrix a(2, 2);
    a(0, 0) = 1;
    a(0, 1) = 2;
    a(1, 0) = 3;
    a(1, 1) = 4;

    CMatrix b(a);

    EXPECT_FLOAT_EQ(b(0, 0), 1);
    EXPECT_FLOAT_EQ(b(0, 1), 2);
    EXPECT_FLOAT_EQ(b(1, 0), 3);
    EXPECT_FLOAT_EQ(b(1, 1), 4);
}

TEST(CMatrixTest, CopyConstructorDeepCopy)
{
    CMatrix a(2, 2);
    a(0, 0) = 10;

    CMatrix b(a);
    a(0, 0) = 50;

    EXPECT_FLOAT_EQ(b(0, 0), 10);
}

TEST(CMatrixTest, MoveConstructor)
{
    CMatrix a(2, 2);
    a(1, 1) = 99;

    CMatrix b(std::move(a));

    EXPECT_FLOAT_EQ(b(1, 1), 99);
}

TEST(CMatrixTest, CopyAssignment)
{
    CMatrix a(2, 2);
    a(0, 0) = 42;

    CMatrix b;
    b = a;

    EXPECT_FLOAT_EQ(b(0, 0), 42);
}

TEST(CMatrixTest, CopyAssignmentDeepCopy)
{
    CMatrix a(2, 2);
    a(0, 0) = 7;

    CMatrix b;
    b = a;

    a(0, 0) = 100;

    EXPECT_FLOAT_EQ(b(0, 0), 7);
}

TEST(CMatrixTest, MoveAssignment)
{
    CMatrix a(2, 2);
    a(1, 0) = 55;

    CMatrix b;
    b = std::move(a);

    EXPECT_FLOAT_EQ(b(1, 0), 55);
}

TEST(CMatrixTest, SelfAssignment)
{
    CMatrix a(2, 2);
    a(0, 1) = 12;

    a = a;

    EXPECT_FLOAT_EQ(a(0, 1), 12);
}

TEST(CMatrixTest, ConstAccess)
{
    CMatrix a(2, 2);
    a(1, 1) = 88;

    const CMatrix& ref = a;

    EXPECT_FLOAT_EQ(ref(1, 1), 88);
}

TEST(CMatrixTest, DefaultValuesAreZero)
{
    CMatrix m(3, 4);

    for (size_t i = 0; i < 3; ++i)
        for (size_t j = 0; j < 4; ++j)
            EXPECT_FLOAT_EQ(m(i, j), 0.0f);
}

TEST(MatrixOpsTest, AdditionBasic)
{
    CMatrix a(2, 2);
    CMatrix b(2, 2);

    a(0, 0) = 1; a(0, 1) = 2;
    a(1, 0) = 3; a(1, 1) = 4;

    b(0, 0) = 5; b(0, 1) = 6;
    b(1, 0) = 7; b(1, 1) = 8;

    CMatrix result = a + b;

    EXPECT_FLOAT_EQ(result(0, 0), 6);
    EXPECT_FLOAT_EQ(result(0, 1), 8);
    EXPECT_FLOAT_EQ(result(1, 0), 10);
    EXPECT_FLOAT_EQ(result(1, 1), 12);
}

TEST(MatrixOpsTest, SubtractionBasic)
{
    CMatrix a(2, 2);
    CMatrix b(2, 2);

    a(0, 0) = 10; a(0, 1) = 20;
    a(1, 0) = 30; a(1, 1) = 40;

    b(0, 0) = 1; b(0, 1) = 2;
    b(1, 0) = 3; b(1, 1) = 4;

    CMatrix result = a - b;

    EXPECT_FLOAT_EQ(result(0, 0), 9);
    EXPECT_FLOAT_EQ(result(0, 1), 18);
    EXPECT_FLOAT_EQ(result(1, 0), 27);
    EXPECT_FLOAT_EQ(result(1, 1), 36);
}

TEST(MatrixOpsTest, AdditionWithZeros)
{
    CMatrix a(2, 2);
    CMatrix b(2, 2);

    CMatrix result = a + b;

    EXPECT_FLOAT_EQ(result(0, 0), 0);
    EXPECT_FLOAT_EQ(result(1, 1), 0);
}

TEST(MatrixOpsTest, OperatorPlusEquals)
{
    CMatrix a(2, 2);
    CMatrix b(2, 2);

    a(0, 0) = 1; a(1, 1) = 2;
    b(0, 0) = 3; b(1, 1) = 4;

    a += b;

    EXPECT_FLOAT_EQ(a(0, 0), 4);
    EXPECT_FLOAT_EQ(a(1, 1), 6);
}

TEST(MatrixOpsTest, OperatorMinusEquals)
{
    CMatrix a(2, 2);
    CMatrix b(2, 2);

    a(0, 0) = 10; a(1, 1) = 20;
    b(0, 0) = 3;  b(1, 1) = 5;

    a -= b;

    EXPECT_FLOAT_EQ(a(0, 0), 7);
    EXPECT_FLOAT_EQ(a(1, 1), 15);
}

TEST(MatrixOpsTest, PlusEqualsChain)
{
    CMatrix a(2, 2);
    CMatrix b(2, 2);

    a(0, 0) = 1;
    b(0, 0) = 2;
    
    (a += b) += b;

    EXPECT_FLOAT_EQ(a(0, 0), 5);
}

TEST(MatrixOpsTest, MinusEqualsChain)
{
    CMatrix a(2, 2);
    CMatrix b(2, 2);

    a(0, 0) = 10;
    b(0, 0) = 3;

    (a -= b) -= b;

    EXPECT_FLOAT_EQ(a(0, 0), 4);
}

TEST(MatrixOpsTest, AdditionDoesNotModifyOperands)
{
    CMatrix a(2, 2);
    CMatrix b(2, 2);

    a(0, 0) = 1;
    b(0, 0) = 2;

    CMatrix result = a + b;

    EXPECT_FLOAT_EQ(a(0, 0), 1);
    EXPECT_FLOAT_EQ(b(0, 0), 2);
    EXPECT_FLOAT_EQ(result(0, 0), 3);
}

TEST(MatrixOpsTest, SubtractionDoesNotModifyOperands)
{
    CMatrix a(2, 2);
    CMatrix b(2, 2);

    a(0, 0) = 5;
    b(0, 0) = 3;

    CMatrix result = a - b;

    EXPECT_FLOAT_EQ(a(0, 0), 5);
    EXPECT_FLOAT_EQ(b(0, 0), 3);
    EXPECT_FLOAT_EQ(result(0, 0), 2);
}

TEST(MatrixMulTest, Basic2x2)
{
    CMatrix a(2, 2);
    CMatrix b(2, 2);

    a(0, 0) = 1; a(0, 1) = 2;
    a(1, 0) = 3; a(1, 1) = 4;

    b(0, 0) = 5; b(0, 1) = 6;
    b(1, 0) = 7; b(1, 1) = 8;

    CMatrix r = a * b;

    EXPECT_FLOAT_EQ(r(0, 0), 19);
    EXPECT_FLOAT_EQ(r(0, 1), 22);
    EXPECT_FLOAT_EQ(r(1, 0), 43);
    EXPECT_FLOAT_EQ(r(1, 1), 50);
}

TEST(MatrixMulTest, RectangularMatrices)
{
    CMatrix a(2, 3);
    CMatrix b(3, 2);

    // a
    // 1 2 3
    // 4 5 6
    a(0, 0) = 1; a(0, 1) = 2; a(0, 2) = 3;
    a(1, 0) = 4; a(1, 1) = 5; a(1, 2) = 6;

    // b
    // 7 8
    // 9 10
    // 11 12
    b(0, 0) = 7;  b(0, 1) = 8;
    b(1, 0) = 9;  b(1, 1) = 10;
    b(2, 0) = 11; b(2, 1) = 12;

    CMatrix r = a * b;

    EXPECT_FLOAT_EQ(r(0, 0), 58);
    EXPECT_FLOAT_EQ(r(0, 1), 64);
    EXPECT_FLOAT_EQ(r(1, 0), 139);
    EXPECT_FLOAT_EQ(r(1, 1), 154);
}

TEST(MatrixMulTest, MultiplyByIdentity)
{
    CMatrix a(2, 2);
    CMatrix I(2, 2);

    a(0, 0) = 5; a(0, 1) = 7;
    a(1, 0) = 1; a(1, 1) = 3;

    I(0, 0) = 1; I(1, 1) = 1;
    I(0, 1) = 0; I(1, 0) = 0;

    CMatrix r = a * I;

    EXPECT_FLOAT_EQ(r(0, 0), 5);
    EXPECT_FLOAT_EQ(r(0, 1), 7);
    EXPECT_FLOAT_EQ(r(1, 0), 1);
    EXPECT_FLOAT_EQ(r(1, 1), 3);
}

TEST(MatrixMulTest, MultiplyByZeroMatrix)
{
    CMatrix a(2, 2);
    CMatrix zero(2, 2);

    a(0, 0) = 1; a(0, 1) = 2;
    a(1, 0) = 3; a(1, 1) = 4;

    CMatrix r = a * zero;

    EXPECT_FLOAT_EQ(r(0, 0), 0);
    EXPECT_FLOAT_EQ(r(0, 1), 0);
    EXPECT_FLOAT_EQ(r(1, 0), 0);
    EXPECT_FLOAT_EQ(r(1, 1), 0);
}

TEST(MatrixMulTest, DoesNotModifyOperands)
{
    CMatrix a(2, 2);
    CMatrix b(2, 2);

    a(0, 0) = 2;
    b(0, 0) = 3;

    CMatrix r = a * b;

    EXPECT_FLOAT_EQ(a(0, 0), 2);
    EXPECT_FLOAT_EQ(b(0, 0), 3);
}

TEST(MatrixMulTest, MultiplyEquals)
{
    CMatrix a(2, 2);
    CMatrix b(2, 2);

    a(0, 0) = 1; a(0, 1) = 2;
    a(1, 0) = 3; a(1, 1) = 4;

    b(0, 0) = 2; b(0, 1) = 0;
    b(1, 0) = 1; b(1, 1) = 2;

    a *= b;

    EXPECT_FLOAT_EQ(a(0, 0), 4);
    EXPECT_FLOAT_EQ(a(0, 1), 4);
    EXPECT_FLOAT_EQ(a(1, 0), 10);
    EXPECT_FLOAT_EQ(a(1, 1), 8);
}

TEST(MatrixMulTest, MultiplyEqualsChain)
{
    CMatrix a(2, 2);
    CMatrix b(2, 2);

    a(0, 0) = 1; a(1, 1) = 1;

    b(0, 0) = 2; b(1, 1) = 2;

    (a *= b) *= b;

    EXPECT_FLOAT_EQ(a(0, 0), 4);
    EXPECT_FLOAT_EQ(a(1, 1), 4);
}

TEST(MatrixMulTest, SizeMismatchDeath)
{
    CMatrix a(2, 3);
    CMatrix b(2, 2);

    EXPECT_DEATH(a * b, "");
}

TEST(MatrixMulTest, InitializationToZero)
{
    CMatrix a(1, 1);
    CMatrix b(1, 1);

    a(0, 0) = 2;
    b(0, 0) = 3;

    CMatrix r = a * b;

    EXPECT_FLOAT_EQ(r(0, 0), 6);
}

TEST(MatrixTransposeTest, Basic2x2)
{
    CMatrix m(2, 2);

    m(0, 0) = 1; m(0, 1) = 2;
    m(1, 0) = 3; m(1, 1) = 4;

    CMatrix t = m.Transpose();

    EXPECT_FLOAT_EQ(t(0, 0), 1);
    EXPECT_FLOAT_EQ(t(0, 1), 3);
    EXPECT_FLOAT_EQ(t(1, 0), 2);
    EXPECT_FLOAT_EQ(t(1, 1), 4);
}

TEST(MatrixTransposeTest, RectangularMatrix)
{
    CMatrix m(2, 3);

    // 1 2 3
    // 4 5 6
    m(0, 0) = 1; m(0, 1) = 2; m(0, 2) = 3;
    m(1, 0) = 4; m(1, 1) = 5; m(1, 2) = 6;

    CMatrix t = m.Transpose();

    // оч≥куЇмо 3x2
    EXPECT_FLOAT_EQ(t(0, 0), 1);
    EXPECT_FLOAT_EQ(t(0, 1), 4);

    EXPECT_FLOAT_EQ(t(1, 0), 2);
    EXPECT_FLOAT_EQ(t(1, 1), 5);

    EXPECT_FLOAT_EQ(t(2, 0), 3);
    EXPECT_FLOAT_EQ(t(2, 1), 6);
}

TEST(MatrixTransposeTest, DoubleTranspose)
{
    CMatrix m(2, 3);

    m(0, 0) = 1; m(0, 1) = 2; m(0, 2) = 3;
    m(1, 0) = 4; m(1, 1) = 5; m(1, 2) = 6;

    CMatrix t = m.Transpose().Transpose();

    for (size_t i = 0; i < 2; ++i)
        for (size_t j = 0; j < 3; ++j)
            EXPECT_FLOAT_EQ(t(i, j), m(i, j));
}

TEST(MatrixTransposeTest, ColumnVectorToRow)
{
    CMatrix m(3, 1);

    m(0, 0) = 1;
    m(1, 0) = 2;
    m(2, 0) = 3;

    CMatrix t = m.Transpose();

    EXPECT_FLOAT_EQ(t(0, 0), 1);
    EXPECT_FLOAT_EQ(t(0, 1), 2);
    EXPECT_FLOAT_EQ(t(0, 2), 3);
}

TEST(MatrixTransposeTest, DoesNotModifyOriginal)
{
    CMatrix m(2, 2);

    m(0, 0) = 1;
    m(1, 1) = 2;

    CMatrix t = m.Transpose();

    EXPECT_FLOAT_EQ(m(0, 0), 1);
    EXPECT_FLOAT_EQ(m(1, 1), 2);
}

TEST(SoftmaxTest, Basic)
{
    CMatrix v(3, 1);

    v(0, 0) = 1;
    v(1, 0) = 2;
    v(2, 0) = 3;

    CMatrix r = Softmax(v);

    float sum = r(0, 0) + r(1, 0) + r(2, 0);

    EXPECT_NEAR(sum, 1.0f, 1e-5f);
    EXPECT_GT(r(2, 0), r(1, 0));
    EXPECT_GT(r(1, 0), r(0, 0));
}

TEST(SoftmaxTest, SumIsOne)
{
    CMatrix v(5, 1);

    for (std::size_t i = 0; i < 5; i++)
        v(i, 0) = static_cast<float>(i);

    CMatrix r = Softmax(v);

    float sum = 0.0f;
    for (std::size_t i = 0; i < 5; i++)
        sum += r(i, 0);

    EXPECT_NEAR(sum, 1.0f, 1e-5f);
}

TEST(SoftmaxTest, InvariantToShift)
{
    CMatrix v1(3, 1);
    CMatrix v2(3, 1);

    v1(0, 0) = 1; v1(1, 0) = 2; v1(2, 0) = 3;

    // +100 до вс≥х
    v2(0, 0) = 101; v2(1, 0) = 102; v2(2, 0) = 103;

    CMatrix r1 = Softmax(v1);
    CMatrix r2 = Softmax(v2);

    for (std::size_t i = 0; i < 3; i++)
        EXPECT_NEAR(r1(i, 0), r2(i, 0), 1e-5f);
}

TEST(SoftmaxTest, LargeValuesStability)
{
    CMatrix v(3, 1);

    v(0, 0) = 1000;
    v(1, 0) = 1001;
    v(2, 0) = 1002;

    CMatrix r = Softmax(v);

    float sum = r(0, 0) + r(1, 0) + r(2, 0);

    EXPECT_NEAR(sum, 1.0f, 1e-5f);
}

TEST(SoftmaxTest, SingleElement)
{
    CMatrix v(1, 1);
    v(0, 0) = 42;

    CMatrix r = Softmax(v);

    EXPECT_NEAR(r(0, 0), 1.0f, 1e-6f);
}

TEST(SoftmaxTest, EqualValues)
{
    CMatrix v(4, 1);

    for (std::size_t i = 0; i < 4; i++)
        v(i, 0) = 5.0f;

    CMatrix r = Softmax(v);

    for (std::size_t i = 0; i < 4; i++)
        EXPECT_NEAR(r(i, 0), 0.25f, 1e-5f);
}

TEST(SoftmaxTest, DoesNotModifyInput)
{
    CMatrix v(2, 1);
    v(0, 0) = 1;
    v(1, 0) = 2;

    CMatrix copy = v;

    Softmax(v);

    EXPECT_FLOAT_EQ(v(0, 0), copy(0, 0));
    EXPECT_FLOAT_EQ(v(1, 0), copy(1, 0));
}

TEST(SoftmaxTest, NotColumnVectorDeath)
{
    CMatrix m(2, 2);

    EXPECT_DEATH(Softmax(m), "");
}

TEST(ReLUTest, Basic)
{
    CMatrix m(2, 2);

    m(0, 0) = -1;
    m(0, 1) = 2;
    m(1, 0) = -3;
    m(1, 1) = 4;

    CMatrix r = ReLu(m);

    EXPECT_FLOAT_EQ(r(0, 0), 0);
    EXPECT_FLOAT_EQ(r(0, 1), 2);
    EXPECT_FLOAT_EQ(r(1, 0), 0);
    EXPECT_FLOAT_EQ(r(1, 1), 4);
}

TEST(ReLUTest, AllNegative)
{
    CMatrix m(3, 1);

    m(0, 0) = -1;
    m(1, 0) = -5;
    m(2, 0) = -100;

    CMatrix r = ReLu(m);

    EXPECT_FLOAT_EQ(r(0, 0), 0);
    EXPECT_FLOAT_EQ(r(1, 0), 0);
    EXPECT_FLOAT_EQ(r(2, 0), 0);
}

TEST(ReLUTest, AllPositive)
{
    CMatrix m(2, 2);

    m(0, 0) = 1;
    m(0, 1) = 2;
    m(1, 0) = 3;
    m(1, 1) = 4;

    CMatrix r = ReLu(m);

    EXPECT_FLOAT_EQ(r(0, 0), 1);
    EXPECT_FLOAT_EQ(r(0, 1), 2);
    EXPECT_FLOAT_EQ(r(1, 0), 3);
    EXPECT_FLOAT_EQ(r(1, 1), 4);
}

TEST(ReLUTest, Zeros)
{
    CMatrix m(2, 2);

    CMatrix r = ReLu(m);

    EXPECT_FLOAT_EQ(r(0, 0), 0);
    EXPECT_FLOAT_EQ(r(1, 1), 0);
}

TEST(ReLUTest, MixedValues)
{
    CMatrix m(1, 5);

    m(0, 0) = -10;
    m(0, 1) = -0.1f;
    m(0, 2) = 0;
    m(0, 3) = 0.1f;
    m(0, 4) = 10;

    CMatrix r = ReLu(m);

    EXPECT_FLOAT_EQ(r(0, 0), 0);
    EXPECT_FLOAT_EQ(r(0, 1), 0);
    EXPECT_FLOAT_EQ(r(0, 2), 0);
    EXPECT_FLOAT_EQ(r(0, 3), 0.1f);
    EXPECT_FLOAT_EQ(r(0, 4), 10);
}

TEST(ReLUTest, DoesNotModifyInput)
{
    CMatrix m(2, 2);

    m(0, 0) = -1;
    m(1, 1) = 5;

    CMatrix copy = m;

    ReLu(m);

    EXPECT_FLOAT_EQ(m(0, 0), copy(0, 0));
    EXPECT_FLOAT_EQ(m(1, 1), copy(1, 1));
}

TEST(ReLUTest, LargeMatrix)
{
    CMatrix m(50, 50);

    for (size_t i = 0; i < 50; i++)
        for (size_t j = 0; j < 50; j++)
            m(i, j) = (i % 2 == 0) ? -1.0f : 1.0f;

    CMatrix r = ReLu(m);

    for (size_t i = 0; i < 50; i++)
        for (size_t j = 0; j < 50; j++)
        {
            if (i % 2 == 0)
                EXPECT_FLOAT_EQ(r(i, j), 0);
            else
                EXPECT_FLOAT_EQ(r(i, j), 1.0f);
        }
}

TEST(ReLUTest, Idempotent)
{
    CMatrix m(2, 2);

    m(0, 0) = -1;
    m(0, 1) = 2;

    CMatrix r1 = ReLu(m);
    CMatrix r2 = ReLu(r1);

    EXPECT_FLOAT_EQ(r1(0, 0), r2(0, 0));
    EXPECT_FLOAT_EQ(r1(0, 1), r2(0, 1));
}

TEST(TokenizerTest, BasicBuildAndEncode)
{
    CTokenizer t;
    t.Build("hello world");

    auto tokens = t.Encode("hello world");

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0], 0);
    EXPECT_EQ(tokens[1], 1);
}

TEST(TokenizerTest, DecodeBasic)
{
    CTokenizer t;
    t.Build("hello world");

    auto tokens = t.Encode("hello world");
    std::string text = t.Decode(tokens);

    EXPECT_EQ(text, "hello world");
}

TEST(TokenizerTest, RepeatedWords)
{
    CTokenizer t;
    t.Build("hello world hello");

    auto tokens = t.Encode("hello world hello");

    ASSERT_EQ(tokens.size(), 3);

    EXPECT_EQ(tokens[0], tokens[2]); // hello == hello
}

TEST(TokenizerTest, UniqueVocabulary)
{
    CTokenizer t;
    t.Build("a a a a");

    auto tokens = t.Encode("a a a");

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], tokens[1]);
    EXPECT_EQ(tokens[1], tokens[2]);
}

TEST(TokenizerTest, UnknownWordsIgnored)
{
    CTokenizer t;
    t.Build("hello world");

    auto tokens = t.Encode("hello unknown world");

    ASSERT_EQ(tokens.size(), 2);
}

TEST(TokenizerTest, DecodeUnknownToken)
{
    CTokenizer t;
    t.Build("hello world");

    std::vector<size_t> tokens = { 0, 999, 1 };

    std::string result = t.Decode(tokens);

    EXPECT_EQ(result, "hello world");
}

TEST(TokenizerTest, EmptyInput)
{
    CTokenizer t;
    t.Build("");

    auto tokens = t.Encode("");

    EXPECT_TRUE(tokens.empty());

    std::string text = t.Decode(tokens);
    EXPECT_TRUE(text.empty());
}

TEST(TokenizerTest, MultipleSpaces)
{
    CTokenizer t;
    t.Build("hello   world");

    auto tokens = t.Encode("hello   world");

    ASSERT_EQ(tokens.size(), 2);
}

TEST(TokenizerTest, LastWordHandled)
{
    CTokenizer t;
    t.Build("hello world");

    auto tokens = t.Encode("hello world");

    ASSERT_EQ(tokens.size(), 2);
}

TEST(TokenizerTest, EncodeDoesNotModifyDictionary)
{
    CTokenizer t;
    t.Build("hello");

    auto tokens1 = t.Encode("hello unknown");
    auto tokens2 = t.Encode("hello");

    ASSERT_EQ(tokens2.size(), 1);
}

TEST(EmbeddingTest, DimensionsAfterInit)
{
    CEmbedding emb(10, 5);

    auto vec = emb.GetEmbedding(0);

    EXPECT_EQ(vec.GetRows(), 1);
    EXPECT_EQ(vec.GetCols(), 5);
}

TEST(EmbeddingTest, InitializeRandomFillsValues)
{
    CEmbedding emb(10, 5);
    emb.InitializeRandom();

    auto vec = emb.GetEmbedding(0);

    bool allZero = true;
    for (size_t j = 0; j < vec.GetCols(); j++)
    {
        if (vec(0, j) != 0.0f)
        {
            allZero = false;
            break;
        }
    }

    EXPECT_FALSE(allZero);
}

TEST(EmbeddingTest, ValuesInRange)
{
    CEmbedding emb(10, 5);
    emb.InitializeRandom();

    for (size_t i = 0; i < 10; i++)
    {
        auto vec = emb.GetEmbedding(i);

        for (size_t j = 0; j < vec.GetCols(); j++)
        {
            EXPECT_GE(vec(0, j), -0.1f);
            EXPECT_LE(vec(0, j), 0.1f);
        }
    }
}

TEST(EmbeddingTest, SameTokenSameEmbedding)
{
    CEmbedding emb(10, 5);
    emb.InitializeRandom();

    auto v1 = emb.GetEmbedding(3);
    auto v2 = emb.GetEmbedding(3);

    for (size_t j = 0; j < v1.GetCols(); j++)
    {
        EXPECT_FLOAT_EQ(v1(0, j), v2(0, j));
    }
}

TEST(EmbeddingTest, DifferentTokensDifferentEmbeddings)
{
    CEmbedding emb(10, 5);
    emb.InitializeRandom();

    auto v1 = emb.GetEmbedding(1);
    auto v2 = emb.GetEmbedding(2);

    bool allEqual = true;

    for (size_t j = 0; j < v1.GetCols(); j++)
    {
        if (v1(0, j) != v2(0, j))
        {
            allEqual = false;
            break;
        }
    }

    EXPECT_FALSE(allEqual);
}

TEST(EmbeddingTest, CorrectCopyFromWeights)
{
    CEmbedding emb(3, 4);
    emb.InitializeRandom();

    auto v = emb.GetEmbedding(2);

    EXPECT_EQ(v.GetRows(), 1);
    EXPECT_EQ(v.GetCols(), 4);
}

TEST(EmbeddingTest, BoundaryTokenIDs)
{
    CEmbedding emb(10, 5);
    emb.InitializeRandom();

    auto first = emb.GetEmbedding(0);
    auto last = emb.GetEmbedding(9);

    EXPECT_EQ(first.GetCols(), 5);
    EXPECT_EQ(last.GetCols(), 5);
}

TEST(EmbeddingTest, OutOfBoundsDeathTest)
{
    CEmbedding emb(10, 5);
    emb.InitializeRandom();

    EXPECT_DEATH(emb.GetEmbedding(10), ".*");
}

TEST(AttentionTest, OutputShape)
{
    CAttention attn(4, 2);

    CMatrix X(3, 4); // seq_len=3, dModel=4

    auto out = attn.Forward(X);

    EXPECT_EQ(out.GetRows(), 3);
    EXPECT_EQ(out.GetCols(), 2);
}

TEST(AttentionTest, RunsWithoutCrash)
{
    CAttention attn(8, 4);

    CMatrix X(5, 8);

    EXPECT_NO_THROW({
        auto out = attn.Forward(X);
        });
}

TEST(AttentionTest, AttentionWeightsSumToOne)
{
    CAttention attn(4, 2);

    CMatrix X(3, 4);
    auto out = attn.Forward(X);

    // тут ми не маЇмо доступу до scores напр€му,
    // тому тест непр€мий Ч через стаб≥льн≥сть значень

    for (size_t i = 0; i < out.GetRows(); i++)
    {
        for (size_t j = 0; j < out.GetCols(); j++)
        {
            EXPECT_FALSE(std::isnan(out(i, j)));
            EXPECT_FALSE(std::isinf(out(i, j)));
        }
    }
}

TEST(AttentionTest, IdenticalInputsProduceIdenticalOutputs)
{
    CAttention attn(4, 2);

    CMatrix X(3, 4);

    // вс≥ р€дки однаков≥
    for (size_t i = 0; i < 3; i++)
        for (size_t j = 0; j < 4; j++)
            X(i, j) = 1.0f;

    auto out = attn.Forward(X);

    for (size_t j = 0; j < out.GetCols(); j++)
    {
        EXPECT_FLOAT_EQ(out(0, j), out(1, j));
        EXPECT_FLOAT_EQ(out(1, j), out(2, j));
    }
}

TEST(AttentionTest, ZeroInput)
{
    CAttention attn(4, 2);

    CMatrix X(3, 4); // вс≥ 0

    auto out = attn.Forward(X);

    for (size_t i = 0; i < out.GetRows(); i++)
    {
        for (size_t j = 0; j < out.GetCols(); j++)
        {
            EXPECT_FALSE(std::isnan(out(i, j)));
            EXPECT_FALSE(std::isinf(out(i, j)));
        }
    }
}

TEST(AttentionTest, LargeValuesStability)
{
    CAttention attn(4, 2);

    CMatrix X(3, 4);

    for (size_t i = 0; i < 3; i++)
        for (size_t j = 0; j < 4; j++)
            X(i, j) = 1000.0f;

    auto out = attn.Forward(X);

    for (size_t i = 0; i < out.GetRows(); i++)
    {
        for (size_t j = 0; j < out.GetCols(); j++)
        {
            EXPECT_FALSE(std::isnan(out(i, j)));
            EXPECT_FALSE(std::isinf(out(i, j)));
        }
    }
}

TEST(AttentionTest, DeterministicForSameObject)
{
    CAttention attn(4, 2);

    CMatrix X(3, 4);

    auto out1 = attn.Forward(X);
    auto out2 = attn.Forward(X);

    for (size_t i = 0; i < out1.GetRows(); i++)
    {
        for (size_t j = 0; j < out1.GetCols(); j++)
        {
            EXPECT_FLOAT_EQ(out1(i, j), out2(i, j));
        }
    }
}

TEST(FFLayerTest, OutputShape)
{
    CFFLayer ff(4, 8);

    CMatrix X(3, 4); // seq_len=3, dModel=4

    auto out = ff.Forward(X);

    EXPECT_EQ(out.GetRows(), 3);
    EXPECT_EQ(out.GetCols(), 4);
}

TEST(FFLayerTest, RunsWithoutCrash)
{
    CFFLayer ff(8, 16);

    CMatrix X(5, 8);

    EXPECT_NO_THROW({
        auto out = ff.Forward(X);
        });
}

TEST(FFLayerTest, DeterministicOutput)
{
    CFFLayer ff(4, 8);

    CMatrix X(3, 4);

    auto out1 = ff.Forward(X);
    auto out2 = ff.Forward(X);

    for (size_t i = 0; i < out1.GetRows(); i++)
    {
        for (size_t j = 0; j < out1.GetCols(); j++)
        {
            EXPECT_FLOAT_EQ(out1(i, j), out2(i, j));
        }
    }
}

TEST(FFLayerTest, ZeroInput)
{
    CFFLayer ff(4, 8);

    CMatrix X(3, 4); // вс≥ 0

    auto out = ff.Forward(X);

    for (size_t i = 0; i < out.GetRows(); i++)
    {
        for (size_t j = 0; j < out.GetCols(); j++)
        {
            EXPECT_FALSE(std::isnan(out(i, j)));
            EXPECT_FALSE(std::isinf(out(i, j)));
        }
    }
}

TEST(FFLayerTest, ReLUActivationEffect)
{
    CFFLayer ff(2, 4);

    CMatrix X(1, 2);

    // велик≥ негативн≥ значенн€
    X(0, 0) = -100.0f;
    X(0, 1) = -100.0f;

    auto out = ff.Forward(X);

    for (size_t j = 0; j < out.GetCols(); j++)
    {
        EXPECT_FALSE(std::isnan(out(0, j)));
        EXPECT_FALSE(std::isinf(out(0, j)));
    }
}

TEST(FFLayerTest, IdenticalRows)
{
    CFFLayer ff(4, 8);

    CMatrix X(3, 4);

    for (size_t i = 0; i < 3; i++)
        for (size_t j = 0; j < 4; j++)
            X(i, j) = 1.0f;

    auto out = ff.Forward(X);

    for (size_t j = 0; j < out.GetCols(); j++)
    {
        EXPECT_FLOAT_EQ(out(0, j), out(1, j));
        EXPECT_FLOAT_EQ(out(1, j), out(2, j));
    }
}

TEST(FFLayerTest, DifferentInputs)
{
    CFFLayer ff(4, 8);

    CMatrix X(2, 4);

    for (size_t j = 0; j < 4; j++)
    {
        X(0, j) = 1.0f;
        X(1, j) = 2.0f;
    }

    auto out = ff.Forward(X);

    bool equal = true;
    for (size_t j = 0; j < out.GetCols(); j++)
    {
        if (out(0, j) != out(1, j))
        {
            equal = false;
            break;
        }
    }

    EXPECT_FALSE(equal);
}

TEST(FFLayerTest, LargeValuesStability)
{
    CFFLayer ff(4, 8);

    CMatrix X(3, 4);

    for (size_t i = 0; i < 3; i++)
        for (size_t j = 0; j < 4; j++)
            X(i, j) = 1000.0f;

    auto out = ff.Forward(X);

    for (size_t i = 0; i < out.GetRows(); i++)
    {
        for (size_t j = 0; j < out.GetCols(); j++)
        {
            EXPECT_FALSE(std::isnan(out(i, j)));
            EXPECT_FALSE(std::isinf(out(i, j)));
        }
    }
}

TEST(FFLayerTest, MinimalDimensions)
{
    CFFLayer ff(1, 1);

    CMatrix X(1, 1);
    X(0, 0) = 1.0f;

    auto out = ff.Forward(X);

    EXPECT_EQ(out.GetRows(), 1);
    EXPECT_EQ(out.GetCols(), 1);
}

TEST(TransformerBlockTest, OutputShape)
{
    CTransformerBlock block(4, 4, 8); // dK == dModel

    CMatrix X(3, 4);

    auto out = block.Forward(X);

    EXPECT_EQ(out.GetRows(), 3);
    EXPECT_EQ(out.GetCols(), 4);
}

TEST(TransformerBlockTest, RunsWithoutCrash)
{
    CTransformerBlock block(8, 8, 16);

    CMatrix X(5, 8);

    EXPECT_NO_THROW({
        auto out = block.Forward(X);
        });
}

TEST(TransformerBlockTest, Deterministic)
{
    CTransformerBlock block(4, 4, 8);

    CMatrix X(3, 4);

    auto out1 = block.Forward(X);
    auto out2 = block.Forward(X);

    for (size_t i = 0; i < out1.GetRows(); i++)
    {
        for (size_t j = 0; j < out1.GetCols(); j++)
        {
            EXPECT_FLOAT_EQ(out1(i, j), out2(i, j));
        }
    }
}

TEST(TransformerBlockTest, ResidualConnectionPreservesSignal)
{
    CTransformerBlock block(4, 4, 8);

    CMatrix X(2, 4);

    for (size_t i = 0; i < 2; i++)
        for (size_t j = 0; j < 4; j++)
            X(i, j) = 1.0f;

    auto out = block.Forward(X);

    bool allZero = true;

    for (size_t i = 0; i < out.GetRows(); i++)
    {
        for (size_t j = 0; j < out.GetCols(); j++)
        {
            if (out(i, j) != 0.0f)
            {
                allZero = false;
                break;
            }
        }
    }

    EXPECT_FALSE(allZero);
}

TEST(TransformerBlockTest, IdenticalRows)
{
    CTransformerBlock block(4, 4, 8);

    CMatrix X(3, 4);

    for (size_t i = 0; i < 3; i++)
        for (size_t j = 0; j < 4; j++)
            X(i, j) = 2.0f;

    auto out = block.Forward(X);

    for (size_t j = 0; j < out.GetCols(); j++)
    {
        EXPECT_FLOAT_EQ(out(0, j), out(1, j));
        EXPECT_FLOAT_EQ(out(1, j), out(2, j));
    }
}

TEST(TransformerBlockTest, DifferentInputs)
{
    CTransformerBlock block(4, 4, 8);

    CMatrix X(2, 4);

    for (size_t j = 0; j < 4; j++)
    {
        X(0, j) = 1.0f;
        X(1, j) = 3.0f;
    }

    auto out = block.Forward(X);

    bool equal = true;

    for (size_t j = 0; j < out.GetCols(); j++)
    {
        if (out(0, j) != out(1, j))
        {
            equal = false;
            break;
        }
    }

    EXPECT_FALSE(equal);
}

TEST(TransformerBlockTest, ZeroInput)
{
    CTransformerBlock block(4, 4, 8);

    CMatrix X(3, 4); // вс≥ 0

    auto out = block.Forward(X);

    for (size_t i = 0; i < out.GetRows(); i++)
    {
        for (size_t j = 0; j < out.GetCols(); j++)
        {
            EXPECT_FALSE(std::isnan(out(i, j)));
            EXPECT_FALSE(std::isinf(out(i, j)));
        }
    }
}

TEST(TransformerBlockTest, LargeValuesStability)
{
    CTransformerBlock block(4, 4, 8);

    CMatrix X(3, 4);

    for (size_t i = 0; i < 3; i++)
        for (size_t j = 0; j < 4; j++)
            X(i, j) = 1000.0f;

    auto out = block.Forward(X);

    for (size_t i = 0; i < out.GetRows(); i++)
    {
        for (size_t j = 0; j < out.GetCols(); j++)
        {
            EXPECT_FALSE(std::isnan(out(i, j)));
            EXPECT_FALSE(std::isinf(out(i, j)));
        }
    }
}

TEST(TransformerBlockTest, MinimalCase)
{
    CTransformerBlock block(1, 1, 2);

    CMatrix X(1, 1);
    X(0, 0) = 1.0f;

    auto out = block.Forward(X);

    EXPECT_EQ(out.GetRows(), 1);
    EXPECT_EQ(out.GetCols(), 1);
}

TEST(TransformerBlockTest, OutputNotEqualToInput)
{
    CTransformerBlock block(4, 4, 8);

    CMatrix X(2, 4);

    for (size_t i = 0; i < 2; i++)
        for (size_t j = 0; j < 4; j++)
            X(i, j) = 1.0f;

    auto out = block.Forward(X);

    bool identical = true;

    for (size_t i = 0; i < X.GetRows(); i++)
    {
        for (size_t j = 0; j < X.GetCols(); j++)
        {
            if (X(i, j) != out(i, j))
            {
                identical = false;
                break;
            }
        }
    }

    EXPECT_FALSE(identical);
}