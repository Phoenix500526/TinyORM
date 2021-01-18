#include <gtest/gtest.h>

#include "tinyorm.h"
using namespace std;
using namespace tinyorm;
using namespace tinyorm_impl;
using namespace tinyorm_impl::Expression;

class Student {
 public:
  string ID;
  int Age;
  string Name;
  string Grade;
  Nullable<bool> IsMale;
  Nullable<int> MathScores;
  Nullable<int> ScienceScores;
  Nullable<int> EnglishScores;
  REFLECTION("Student", ID, Age, Name, Grade, IsMale, MathScores, ScienceScores,
             EnglishScores);
};

class Teacher {
 public:
  string ID;
  string Name;
  string Grade;
  Nullable<string> Address;
  Nullable<double> Salary;
  REFLECTION("Teacher", ID, Name, Grade, Address, Salary);
};

class TypeSystemUnittest : public ::testing::Test {
 public:
  TypeSystemUnittest()
      : s1{"0001", 22, "Jack", "Third", nullptr, 95, 97, 90},
        t1{"0002", "Rose", "Third", nullptr, 1234.56},
        field{s1, t1} {}
  ~TypeSystemUnittest() = default;

 protected:
  Student s1;
  Teacher t1;
  FieldExtractor field;
};

TEST_F(TypeSystemUnittest, ReflectionTest) {
  EXPECT_EQ(ReflectionVisitor::TableName(s1), string("Student"));
  auto fieldNames = ReflectionVisitor::FieldNames(s1);
  EXPECT_EQ(fieldNames.size(), 8);
  EXPECT_EQ(fieldNames[0], string("ID"));
  EXPECT_EQ(fieldNames[2], string("Name"));
  EXPECT_EQ(fieldNames[4], string("IsMale"));

  const auto& res_1 = field(s1.ID);
  EXPECT_EQ(res_1.fieldName_, string("ID"));
  EXPECT_EQ(*res_1.tableName_, string("Student"));

  const auto& res_2 = field(s1.Grade);
  EXPECT_EQ(res_2.fieldName_, string("Grade"));
  EXPECT_EQ(*res_2.tableName_, string("Student"));

  const auto& res_3 = field(t1.ID);
  EXPECT_EQ(res_3.fieldName_, string("ID"));
  EXPECT_EQ(*res_3.tableName_, string("Teacher"));

  const auto& res_4 = field(t1.Salary);
  EXPECT_EQ(res_4.fieldName_, string("Salary"));
  EXPECT_EQ(*res_4.tableName_, string("Teacher"));
}

TEST_F(TypeSystemUnittest, AssignmentExpressionTest) {
  auto res_1 = field(s1.Age);
  EXPECT_EQ((res_1 = 22).ToString(), string("Age=22"));
  auto res_2 = field(s1.Name);
  EXPECT_EQ((res_2 = "Jack").ToString(), string("Name='Jack'"));
  auto res_3 = field(t1.Address);
  EXPECT_EQ((res_3 = "Room 201, 18 Changjiang Road, Wujing District, Changzhou "
                     "City, Jiangsu Province")
                .ToString(),
            string("Address='Room 201, 18 Changjiang Road, Wujing District, "
                   "Changzhou City, Jiangsu Province'"));
  auto res_4 = field(t1.Salary);
  EXPECT_EQ((res_4 = 1234.58).ToString(), string("Salary=1234.58"));
  auto res_5 = field(s1.IsMale);
  EXPECT_EQ((res_5 = nullptr).ToString(), string("IsMale=null"));
  EXPECT_EQ(((res_1 = 25) && (res_2 = "Phoenix")).ToString(),
            string("Age=25,Name='Phoenix'"));
}

TEST_F(TypeSystemUnittest, RelationExpressionTest) {
  auto s1_Age = field(s1.Age);
  EXPECT_EQ((s1_Age == 22).ToString(), string("Student.Age=22"));
  EXPECT_EQ((s1_Age != 22).ToString(), string("Student.Age!=22"));
  EXPECT_EQ((s1_Age < 22).ToString(), string("Student.Age<22"));
  EXPECT_EQ((s1_Age <= 22).ToString(), string("Student.Age<=22"));
  EXPECT_EQ((s1_Age > 22).ToString(), string("Student.Age>22"));
  EXPECT_EQ((s1_Age >= 22).ToString(), string("Student.Age>=22"));

  auto s1_Name = field(s1.Name);
  EXPECT_EQ((s1_Name == string("Jack")).ToString(),
            string("Student.Name='Jack'"));
  EXPECT_EQ((s1_Name != string("Jack")).ToString(),
            string("Student.Name!='Jack'"));
  EXPECT_EQ((s1_Name & "Ja%").ToString(), string("Student.Name like 'Ja%'"));
  EXPECT_EQ((s1_Name | "Ja%").ToString(),
            string("Student.Name not like 'Ja%'"));

  auto s1_math = field(s1.MathScores);
  auto s1_science = field(s1.ScienceScores);
  EXPECT_EQ((s1_math == s1_science).ToString(),
            string("Student.MathScores=Student.ScienceScores"));
  EXPECT_EQ((s1_math != s1_science).ToString(),
            string("Student.MathScores!=Student.ScienceScores"));
  EXPECT_EQ((s1_math > s1_science).ToString(),
            string("Student.MathScores>Student.ScienceScores"));
  EXPECT_EQ((s1_math >= s1_science).ToString(),
            string("Student.MathScores>=Student.ScienceScores"));
  EXPECT_EQ((s1_math < s1_science).ToString(),
            string("Student.MathScores<Student.ScienceScores"));
  EXPECT_EQ((s1_math <= s1_science).ToString(),
            string("Student.MathScores<=Student.ScienceScores"));
  EXPECT_EQ((s1_math == nullptr).ToString(),
            string("Student.MathScores is null"));
  EXPECT_EQ((s1_math != nullptr).ToString(),
            string("Student.MathScores is not null"));
}

TEST_F(TypeSystemUnittest, AggregateExpressionTest) {
  auto s1_math = field(s1.MathScores);
  EXPECT_EQ(Count().ToString(), string("count (*)"));
  EXPECT_EQ(Count(s1_math).ToString(), string("count(Student.MathScores)"));
  EXPECT_EQ(Max(s1_math).ToString(), string("max(Student.MathScores)"));
  EXPECT_EQ(Min(s1_math).ToString(), string("min(Student.MathScores)"));
  EXPECT_EQ(Avg(s1_math).ToString(), string("avg(Student.MathScores)"));
  EXPECT_EQ(Sum(s1_math).ToString(), string("sum(Student.MathScores)"));
}