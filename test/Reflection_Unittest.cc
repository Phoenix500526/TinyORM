#include <gtest/gtest.h>

#include "tinyorm.h"
using namespace std;
using namespace tinyorm;
using namespace tinyorm_impl;

class Student {
 public:
  string ID;
  int Age;
  string Name;
  bool IsMale;
  string Grade;
  REFLECTION("Student", ID, Age, Name, IsMale, Grade);
};

class Teacher {
 public:
  string ID;
  string Name;
  string Grade;
  double Salary;
  REFLECTION("Teacher", ID, Name, Grade, Salary);
};

TEST(ReflectionUnittest, ReflectionTest) {
  Student s1;
  Teacher t1;
  EXPECT_EQ(ReflectionVisitor::TableName(s1), string("Student"));
  auto fieldNames = ReflectionVisitor::FieldNames(s1);
  EXPECT_EQ(fieldNames.size(), 5);
  EXPECT_EQ(fieldNames[0], string("ID"));
  EXPECT_EQ(fieldNames[2], string("Name"));
  EXPECT_EQ(fieldNames[4], string("Grade"));

  FieldExtractor field{s1, t1};
  auto res_1 = field(s1.ID);
  EXPECT_EQ(res_1.first, string("ID"));
  EXPECT_EQ(res_1.second, string("Student"));

  auto res_2 = field(s1.Grade);
  EXPECT_EQ(res_2.first, string("Grade"));
  EXPECT_EQ(res_2.second, string("Student"));

  auto res_3 = field(t1.ID);
  EXPECT_EQ(res_3.first, string("ID"));
  EXPECT_EQ(res_3.second, string("Teacher"));

  auto res_4 = field(t1.Salary);
  EXPECT_EQ(res_4.first, string("Salary"));
  EXPECT_EQ(res_4.second, string("Teacher"));
}