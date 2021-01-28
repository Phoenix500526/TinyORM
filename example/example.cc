#include <iostream>
#include <vector>

#include "tinyorm.h"

using namespace std;
using namespace tinyorm;

namespace PrintHelper {

template <typename T>
void PrintNullable(const Nullable<T>& val) {
    if (val == nullptr)
        cout << "null";
    else
        cout << val.Value();
}

template <typename Tuple, std::size_t N>
struct TuplePrinter {
    static inline void Print(const Tuple& t) {
        TuplePrinter<Tuple, N - 1>::Print(t);
        cout << ", ";
        PrintNullable(std::get<N - 1>(t));
    }
};

template <typename Tuple>
struct TuplePrinter<Tuple, 1> {
    static inline void Print(const Tuple& t) { PrintNullable(std::get<0>(t)); }
};

template <typename... Args>
void PrintTuple(const std::tuple<Args...>& tup) {
    cout << "[ ";
    TuplePrinter<decltype(tup), sizeof...(Args)>::Print(tup);
    cout << "]\n";
}

template <typename Container>
void PrintTuples(const Container& container) {
    for (const auto& item : container) {
        PrintTuple(item);
    }
}

}  // namespace PrintHelper

struct OrderTable {
    string OrderID;
    string Name;
    string Phone;
    string Address;
    string CommodityID;
    int Count;
    Nullable<string> Date;
    REFLECTION("OrderTable", OrderID, Name, Phone, Address, CommodityID, Count,
               Date);
};

struct Warehouse {
    string CommodityID;
    int StockQuantity;
    double Price;
    Nullable<string> DateOfStorage;
    REFLECTION("Warehouse", CommodityID, StockQuantity, Price, DateOfStorage);
};

int main() {
    DBManager<Sqlite3> dbm("tinyorm_example.db");
    OrderTable order;
    Warehouse warehouse;
    auto field = FieldExtractor{order, warehouse};

    try {
        dbm.DropTbl(OrderTable{});
        dbm.DropTbl(Warehouse{});
    } catch (...) {
    }

    /*
     * CREATE TABLE Warehouse(
     *	CommodityID text not null primary key,
     *	StockQuantity integer not null,
     *	Price real not null,
     *	DateOfStorage text,
     *	check ((Warehouse.StockQuantity>=0 and Warehouse.Price>0))
     *);
     */
    dbm.CreateTbl(Warehouse{},
                  Constraint::Check(field(warehouse.StockQuantity) >= 0 &&
                                    field(warehouse.Price) > 0.0));
    vector<Warehouse> stock = {{"00003", 1024, 30.5, nullptr},
                               {"00013", 2048, 88.8, nullptr},
                               {"00010", 3192, 50.0, "2020-12-25"},
                               {"00042", 4096, 100.0},
                               {"00101", 8192, 13.0}};
    dbm.InsertRange(stock);

    // Create a Table
    /*
     * CREATE TABLE OrderTable(
     *	OrderID text not null primary key,
     *	Name text not null,
     *	Phone text not null,
     *	Address text not null,
     *	CommodityID text not null,
     *	Count integer not null default 1,
     *	Date text,
     *	check (OrderTable.Count>0)
     *	foreign key (CommodityID) references Warehouse(CommodityID)
     *);
     */
    dbm.CreateTbl(OrderTable{}, Constraint::Default(field(order.Count), 1),
                  Constraint::Check((field(order.Count)) > 0),
                  Constraint::Reference(field(order.CommodityID),
                                        field(warehouse.CommodityID)));

    vector<OrderTable> buyer_1 = {
        {"000001-1", "Jack", "123456789", "China", "00003", 5, "2021-05-11"},
        {"000001-2", "Jack", "123456789", "China", "00013", 3, "2021-05-11"},
        {"000001-3", "Jack", "123456789", "China", "00010", 2, "2021-05-11"}};
    vector<OrderTable> buyer_2 = {
        {"000002-1", "Rose", "987654321", "USA", "00101", 10, "2021-03-04"},
        {"000002-2", "Rose", "987654321", "USA", "00042", 25, "2021-03-04"}};
    OrderTable buyer_3 = {"000003-1", "David", "0010110123", "UK",
                          "00003",    5,       nullptr};

    /*
     * INSERT INTO OrderTable (OrderTable.OrderID, OrderTable.name,
     * 	OrderTable.Phone, OrderTable.Address, OrderTable.CommodityID,
     * 	OrderTable.Count, OrderTable.Date) VALUES('000001-1', 'Jack',
     * 	'123456789', 'China', '00003', 5, '2021-05-11')
     */
    dbm.InsertRange(buyer_1);
    dbm.InsertRange(buyer_2);
    dbm.Insert(buyer_3);
    buyer_3.Date = "2021-01-01";

    /*
     * UPDATE OrderTable SET OrderTable.Date='2021-01-01' WHERE
     * OrderTable.OrderID='000003-1'
     */
    dbm.Update(buyer_3);
    for (auto& record : buyer_1) {
        record.Count += 3;
    }
    dbm.UpdateRange(buyer_1);

    auto result_1 = dbm.Query(OrderTable{})
                        .Select(field(order.OrderID), field(order.Name),
                                field(order.CommodityID), field(order.Count))
                        .Where(field(order.OrderID) & string("000001-%"))
                        .OrderBy(field(order.Count))
                        .ToVector();
    PrintHelper::PrintTuples(result_1);

    auto result_2 = dbm.Query(OrderTable{})
                        .Join(Warehouse{}, field(order.CommodityID) ==
                                               field(warehouse.CommodityID));
    auto result_3 = result_2
                        .Select(field(order.Name), field(order.Count),
                                field(warehouse.Price))
                        .Where(field(order.Name) == string("Rose"))
                        .ToVector();
    PrintHelper::PrintTuples(result_3);
    /*
     * DELETE FROM OrderTable where OrderTable.OrderId like '000001%';
     */
    dbm.Delete(OrderTable{}, field(order.OrderID) & string("000001%"));

    return 0;
}