#include <gtest/gtest.h>

#include "tinyorm.h"

using namespace std;
using namespace tinyorm;
using namespace tinyorm_impl;

class TypesUnittest : public ::testing::Test {
public:
    TypesUnittest()
        : var_1(), var_2(nullptr), var_3(3.14159), var_4("HelloWorld") {}
    ~TypesUnittest() = default;

protected:
    Nullable<int> var_1;
    Nullable<double> var_2;
    Nullable<double> var_3;
    Nullable<string> var_4;
};

struct SerializeTest {
    SerializeTest()
        : a(0), b(10), c(100), d(2.71), e(3.14), str("Hello"), nothing() {}
    int a;
    long b;
    long long c;
    float d;
    double e;
    Nullable<string> str;
    Nullable<string> nothing;
    REFLECTION("SerializeTest", a, b, c, d, e, str, nothing);
};

TEST_F(TypesUnittest, NullableConstructTest) {
    EXPECT_EQ(var_1.HasValue(), false);
    EXPECT_EQ(var_2.HasValue(), false);
    EXPECT_EQ(var_3.HasValue(), true);
    EXPECT_EQ(var_3.Value(), 3.14159);
    auto tmp_1 = var_3;
    EXPECT_EQ(tmp_1.HasValue(), true);
    EXPECT_EQ(tmp_1.Value(), 3.14159);
    auto tmp_2 = std::move(tmp_1);
    EXPECT_EQ(tmp_2.HasValue(), true);
    EXPECT_EQ(tmp_2.Value(), 3.14159);
    var_2 = 2.71;
    EXPECT_EQ(var_2.HasValue(), true);
    EXPECT_EQ(var_2.Value(), 2.71);
    var_3 = 2;
    EXPECT_EQ(var_3.HasValue(), true);
    EXPECT_EQ(var_3.Value(), 2.0);
    var_1 = 3.14;
    EXPECT_EQ(var_1.HasValue(), true);
    EXPECT_EQ(var_1.Value(), 3);
    EXPECT_EQ(var_4.HasValue(), true);
    EXPECT_EQ(var_4.Value(), string("HelloWorld"));
    Nullable<string> tmp_3 = "HelloWorld";
    EXPECT_TRUE(tmp_3.HasValue());
    EXPECT_EQ(tmp_3.Value(), string("HelloWorld"));
    tmp_3 = "World";
    EXPECT_TRUE(tmp_3.HasValue());
    EXPECT_EQ(tmp_3.Value(), string("World"));
    tmp_3 = "";
    EXPECT_FALSE(tmp_3.HasValue());
    EXPECT_TRUE(tmp_3 == nullptr);
}

TEST_F(TypesUnittest, NullableCompareTest) {
    Nullable<int> tmp_1(nullptr);
    EXPECT_TRUE(var_1 == tmp_1);
    tmp_1 = 10;
    EXPECT_FALSE(var_1 == tmp_1);
    var_1 = tmp_1;
    EXPECT_TRUE(var_1 == tmp_1);
    auto str = string("HelloWorld");
    EXPECT_TRUE(var_4 == str && str == var_4);
    EXPECT_TRUE(var_2 == nullptr && nullptr == var_2);
}

TEST_F(TypesUnittest, TypeStrTest) {
    bool a;
    char b;
    int c;
    long d;
    long long e;
    EXPECT_STREQ(TypeString<decltype(a)>::type_string, " integer");
    // EXPECT_STREQ(TypeString<decltype(b)>::type_string," integer"); //compile
    // error
    EXPECT_STREQ(TypeString<decltype(c)>::type_string, " integer");
    EXPECT_STREQ(TypeString<decltype(d)>::type_string, " integer");
    EXPECT_STREQ(TypeString<decltype(e)>::type_string, " integer");

    float h;
    double i;
    long double j;
    EXPECT_STREQ(TypeString<decltype(h)>::type_string, " real");
    EXPECT_STREQ(TypeString<decltype(i)>::type_string, " real");
    EXPECT_STREQ(TypeString<decltype(j)>::type_string, " real");

    string str;
    EXPECT_STREQ(TypeString<decltype(str)>::type_string, " text");

    EXPECT_STREQ(TypeString<decltype(var_1)>::type_string, " integer");
    EXPECT_STREQ(TypeString<decltype(var_2)>::type_string, " real");
    EXPECT_STREQ(TypeString<decltype(var_4)>::type_string, " text");
}

TEST_F(TypesUnittest, TypeSerializerTest) {
    SerializeTest st;
    std::ostringstream os;
    ReflectionVisitor::Visit(st, [&os](const auto&... value) {
        (Serializer::Serialize(os << ' ', value), ...);
    });
    EXPECT_EQ(os.str(), string(" 0 10 100 2.71 3.14 'Hello' "));
    int a = 1;
    float f = 4.77;
    string str("GTEST");
    os.str("");
    Serializer::Serialize(os, a);
    string res_a = os.str();
    os.str("");
    EXPECT_EQ(res_a, string("1"));

    Serializer::Serialize(os, f);
    string res_f = os.str();
    os.str("");
    EXPECT_EQ(res_f, string("4.77"));

    Serializer::Serialize(os, str);
    string res_str = os.str();
    os.str("");
    EXPECT_EQ(res_str, string("'GTEST'"));

    int des_a;
    float des_f;
    string des_str;
    Deserializer::Deserialize(des_a, res_a.c_str());
    EXPECT_EQ(des_a, a);
    Deserializer::Deserialize(des_f, res_f.c_str());
    EXPECT_EQ(des_f, f);
}
