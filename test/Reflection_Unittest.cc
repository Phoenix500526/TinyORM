#include <gtest/gtest.h>

#include <unordered_map>

#include "tinyorm.h"
using namespace std;
using namespace tinyorm;
using namespace tinyorm_impl;
using namespace tinyorm_impl::Expression;

unordered_map<string, string> result;

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
    REFLECTION("Student", ID, Age, Name, Grade, IsMale, MathScores,
               ScienceScores, EnglishScores);
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

class dummy {
private:
    string db;

public:
    dummy(const string& str) : db(str) {}
    ~dummy() = default;
    void Execute(const string& cmd) {
        size_t first_space = cmd.find_first_of(' ');
        string str = std::move(cmd.substr(0, first_space));
        result[str] = cmd;
    }

    void ExecuteCallback(const string& cmd,
                         std::function<void(int, char**)> callback) {
        Execute(cmd);
    }
};

class TypeSystemUnittest : public ::testing::Test {
    friend class dummy;

public:
    TypeSystemUnittest()
        : s1{"0001", 22, "Jack", "2-nd", nullptr, 95, 97, 90},
          t1{"0002", "Rose", "2-nd", nullptr, 1234.56},
          field{s1, t1},
          dbm{"dummy"} {}
    ~TypeSystemUnittest() = default;

protected:
    Student s1;
    Teacher t1;
    FieldExtractor field;
    DBManager<dummy> dbm;
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
    EXPECT_EQ(
        (res_3 = "Room 201, 18 Changjiang Road, Wujing District, Changzhou "
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

TEST_F(TypeSystemUnittest, CalculationExpressionTest) {
    EXPECT_EQ((field(s1.Age) + 10).ToString(), string("(Student.Age+10)"));
    EXPECT_EQ((field(s1.Age) - 10).ToString(), string("(Student.Age-10)"));
    EXPECT_EQ((field(s1.Age) * 10).ToString(), string("(Student.Age*10)"));
    EXPECT_EQ((field(s1.Age) / 10).ToString(), string("(Student.Age/10)"));
    EXPECT_EQ((field(s1.Age) % 10).ToString(), string("(Student.Age%10)"));

    EXPECT_EQ((10 + field(s1.Age)).ToString(), string("(10+Student.Age)"));
    EXPECT_EQ((10 - field(s1.Age)).ToString(), string("(10-Student.Age)"));
    EXPECT_EQ((10 * field(s1.Age)).ToString(), string("(10*Student.Age)"));
    EXPECT_EQ((10 / field(s1.Age)).ToString(), string("(10/Student.Age)"));
    EXPECT_EQ((10 % field(s1.Age)).ToString(), string("(10%Student.Age)"));

    string calculation_column =
        "(Student.MathScores+((Student.EnglishScores*0.800000)+(Student."
        "ScienceScores*0.700000)))";
    auto res = field(s1.MathScores) +
               (field(s1.EnglishScores) * 0.8 + field(s1.ScienceScores) * 0.7);
    EXPECT_EQ(res.ToString(), calculation_column);

    string sum_of_calculation_column =
        "sum((Student.MathScores+((Student.EnglishScores*0.800000)+(Student."
        "ScienceScores*0.700000))))";
    EXPECT_EQ(Sum(res).ToString(), sum_of_calculation_column);
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

TEST_F(TypeSystemUnittest, DBManagerTest) {
    string create_str =
        "create table Student("
        "ID text not null primary key,"
        "Age integer not null default 22,"
        "Name text not null,Grade text not null,IsMale integer,MathScores "
        "integer,ScienceScores "
        "integer,EnglishScores integer,"
        "check ((Student.Age>20 and Student.Age<30)),"
        "unique (MathScores,EnglishScores),"
        "foreign key (ID,Name) references Teacher(ID,Name)"
        ");";
    dbm.CreateTbl(
        s1, Constraint::Default(field(s1.Age), 22),
        Constraint::Check(field(s1.Age) > 20 && field(s1.Age) < 30),
        Constraint::Unique(Constraint::CompositeField{field(s1.MathScores),
                                                      field(s1.EnglishScores)}),
        Constraint::Reference(
            Constraint::CompositeField{field(s1.ID), field(s1.Name)},
            Constraint::CompositeField{field(t1.ID), field(t1.Name)}));
    EXPECT_EQ(result.at("create"), create_str);
    dbm.DropTbl(s1);
    string drop_str = "drop table Student;";
    EXPECT_EQ(result.at("drop"), drop_str);
    string delete_str = "delete from Student where ID='0001';";
    dbm.Delete(s1);
    EXPECT_EQ(result.at("delete"), delete_str);
    result.erase("delete");
    dbm.Delete(s1, field(s1.Age) > 20);
    delete_str = "delete from Student where Student.Age>20;";
    EXPECT_EQ(result.at("delete"), delete_str);
    string insert_str =
        "insert into Student("
        "ID,Age,Name,Grade,MathScores,"
        "ScienceScores,EnglishScores) values ("
        "'0001',22,'Jack','2-nd',95,97,90);";
    dbm.Insert(s1);
    EXPECT_EQ(result.at("insert"), insert_str);

    string update_str =
        "update Student "
        "set Age=24,Name='Narutal',Grade='1-st',IsMale=null,"
        "MathScores=60,ScienceScores=60,EnglishScores=60 "
        "where Student.ID='0002';";
    Student s2{"0002", 24, "Narutal", "1-st", nullptr, 60, 60, 60};
    dbm.Update(s2);
    EXPECT_EQ(result.at("update"), update_str);
    result.erase("update");
    update_str =
        "update Student "
        "set Age=27,Name='Phoenix',Grade='3-th',IsMale=1 "
        "where Student.ID='0001';";
    dbm.Update(s1,
               (field(s1.Age) = 27) && (field(s1.Name) = "Phoenix") &&
                   (field(s1.Grade) = "3-th") && (field(s1.IsMale) = 1),
               (field(s1.ID) == string("0001")));
    EXPECT_EQ(result.at("update"), update_str);
    result.clear();
}

TEST_F(TypeSystemUnittest, RangeOperationTest) {
    vector<Student> vec = {{"0003", 21, "Rose", "1-st", false, 90, 92, 93},
                           {"0004", 25, "Dick", "3-th", nullptr, 92, 93, 94}};
    string insert_str =
        "insert into Student("
        "ID,Age,Name,Grade,IsMale,MathScores,"
        "ScienceScores,EnglishScores) values ("
        "'0003',21,'Rose','1-st',0,90,92,93);"
        "insert into Student("
        "ID,Age,Name,Grade,MathScores,"
        "ScienceScores,EnglishScores) values ("
        "'0004',25,'Dick','3-th',92,93,94);";
    dbm.InsertRange(vec);
    EXPECT_EQ(result.at("insert"), insert_str);
    vec[0].Age = 24;
    vec[0].Name = "Narutal";
    vec[0].IsMale = true;
    vec[0].MathScores = 60;
    vec[0].ScienceScores = 60;
    vec[0].EnglishScores = 60;

    vec[1].Age = 27;
    vec[1].Name = "Phoenix";
    vec[1].IsMale = true;
    vec[1].MathScores = 100;
    vec[1].ScienceScores = 100;
    vec[1].EnglishScores = 100;
    string update_str =
        "update Student "
        "set Age=24,Name='Narutal',Grade='1-st',IsMale=1,"
        "MathScores=60,ScienceScores=60,EnglishScores=60 "
        "where Student.ID='0003';"
        "update Student "
        "set Age=27,Name='Phoenix',Grade='3-th',IsMale=1,"
        "MathScores=100,ScienceScores=100,EnglishScores=100 "
        "where Student.ID='0004';";
    dbm.UpdateRange(vec);
    EXPECT_EQ(result.at("update"), update_str);
}

TEST_F(TypeSystemUnittest, QueryTest) {
    string select_sql = "select distinct * from Student;";
    dbm.Query(Student{}).Distinct().ToVector();
    EXPECT_EQ(result.at("select"), select_sql);
    select_sql =
        "select Student.MathScores,Student.ScienceScores,Student.EnglishScores "
        "from Student where (Student.ID='0001');";
    dbm.Query(Student{})
        .Select(field(s1.MathScores), field(s1.ScienceScores),
                field(s1.EnglishScores))
        .Where(field(s1.ID) == string("0001"))
        .ToVector();
    EXPECT_EQ(result.at("select"), select_sql);

    select_sql =
        "select Student.MathScores,Student.ScienceScores,Student.EnglishScores "
        "from Student where (Student.ID='0001') limit 10 offset 3;";
    dbm.Query(Student{})
        .Select(field(s1.MathScores), field(s1.ScienceScores),
                field(s1.EnglishScores))
        .Where(field(s1.ID) == string("0001"))
        .Limit(10)
        .Offset(3)
        .ToVector();
    EXPECT_EQ(result.at("select"), select_sql);

    select_sql =
        "select Student.MathScores,Student.ScienceScores,Student.EnglishScores "
        "from Student limit ~0 offset 3;";
    dbm.Query(Student{})
        .Select(field(s1.MathScores), field(s1.ScienceScores),
                field(s1.EnglishScores))
        .Offset(3)
        .ToVector();
    EXPECT_EQ(result.at("select"), select_sql);

    select_sql =
        "select Student.MathScores,Student.ScienceScores,Student.EnglishScores "
        "from Student group by Student.Grade,Student.IsMale having "
        "(sum(Student.MathScores)>=75 "
        "and "
        "(Student.Grade='1-st' or Student.Grade='2-nd'));";
    dbm.Query(Student{})
        .Select(field(s1.MathScores), field(s1.ScienceScores),
                field(s1.EnglishScores))
        .GroupBy(field(s1.Grade), field(s1.IsMale))
        .Having(Sum(field(s1.MathScores)) >= 75 &&
                (field(s1.Grade) == string("1-st") ||
                 field(s1.Grade) == string("2-nd")))
        .ToVector();
    EXPECT_EQ(result.at("select"), select_sql);

    select_sql =
        "select * from Student order by Student.MathScores,Student.Age;";
    dbm.Query(Student{})
        .OrderBy(field(s1.MathScores), field(s1.Age))
        .ToVector();
    EXPECT_EQ(result.at("select"), select_sql);

    select_sql =
        "select * from Student order by Student.MathScores,Student.Age desc;";
    dbm.Query(Student{})
        .OrderByDescending(field(s1.MathScores), field(s1.Age))
        .ToVector();
    EXPECT_EQ(result.at("select"), select_sql);
}

TEST_F(TypeSystemUnittest, InterTableSelect) {
    string join_str =
        "select * from Student join Teacher on Student.Grade=Teacher.Grade;";
    dbm.Query(Student{})
        .Join(Teacher{}, field(s1.Grade) == field(t1.Grade))
        .ToVector();
    EXPECT_EQ(result.at("select"), join_str);

    join_str =
        "select "
        "Teacher.Name,sum(((Student.MathScores+Student.EnglishScores)+Student."
        "ScienceScores)) "
        "from Student join Teacher on Student.Grade=Teacher.Grade "
        "where (Student.Grade='3-th');";
    dbm.Query(Student{})
        .Join(Teacher{}, field(s1.Grade) == field(t1.Grade))
        .Select(field(t1.Name),
                Sum((field(s1.MathScores) + field(s1.EnglishScores)) +
                    field(s1.ScienceScores)))
        .Where(field(s1.Grade) == string("3-th"))
        .ToVector();
    EXPECT_EQ(result.at("select"), join_str);

    join_str =
        "select * from Teacher left join Student on "
        "Teacher.Grade=Student.Grade;";
    dbm.Query(Teacher{})
        .LeftJoin(Student{}, field(t1.Grade) == field(s1.Grade))
        .ToVector();
    EXPECT_EQ(result.at("select"), join_str);

    /*
     * SELECT OrderTable.Name, OrderTable.Count, Warehouse.Price
     *      FROM OrderTable JOIN Warehouse
     *      ON OrderTable.CommodityID=Warehouse.CommodityID
     *      WHERE OrderTable.Name='Rose';
     */

    string union_str =
        "select * from Student join Teacher "
        "on Student.Grade=Teacher.Grade "
        "union "
        "select * from Student left join Teacher "
        "on Student.Grade=Teacher.Grade;";
    auto joinedQuery = dbm.Query(Student{}).LeftJoin(
        Teacher{}, field(s1.Grade) == field(t1.Grade));
    dbm.Query(Student{})
        .Join(Teacher{}, field(s1.Grade) == field(t1.Grade))
        .Union(joinedQuery)
        .ToVector();
    EXPECT_EQ(result.at("select"), union_str);

    union_str =
        "select * from Student join Teacher "
        "on Student.Grade=Teacher.Grade "
        "union all "
        "select * from Student left join Teacher "
        "on Student.Grade=Teacher.Grade;";
    dbm.Query(Student{})
        .Join(Teacher{}, field(s1.Grade) == field(t1.Grade))
        .UnionAll(joinedQuery)
        .ToVector();
    EXPECT_EQ(result.at("select"), union_str);

    string intersect_str =
        "select * from Student join Teacher "
        "on Student.Grade=Teacher.Grade "
        "intersect "
        "select * from Student left join Teacher "
        "on Student.Grade=Teacher.Grade;";
    dbm.Query(Student{})
        .Join(Teacher{}, field(s1.Grade) == field(t1.Grade))
        .Intersect(joinedQuery)
        .ToVector();
    EXPECT_EQ(result.at("select"), intersect_str);

    string except_str =
        "select * from Student join Teacher "
        "on Student.Grade=Teacher.Grade "
        "except "
        "select * from Student left join Teacher "
        "on Student.Grade=Teacher.Grade;";
    dbm.Query(Student{})
        .Join(Teacher{}, field(s1.Grade) == field(t1.Grade))
        .Except(joinedQuery)
        .ToVector();
    EXPECT_EQ(result.at("select"), except_str);
}