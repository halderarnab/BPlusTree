//===----------------------------------------------------------------------===//
//
//                         Rutgers CS539 - Database System
//                         ***DO NO SHARE PUBLICLY***
//
// Identification:   test/b_plus_tree_delete_test.cpp
//
// Copyright (c) 2022, Rutgers University
//
//===----------------------------------------------------------------------===/

#include "b_plus_tree.h"

#include <iostream>
#include <vector>

using std::cout;
using std::endl;
using std::vector;

int main() {
	cout << "B+ Tree Test..." << endl;
	BPlusTree tree;

	if (!tree.IsEmpty()) {
		cout << "ERROR: IsEmpty() test fail!" << endl;
	}
	else {
		cout << "PASS: IsEmpty() test passed!" << endl;
	}

	bool test1_success = true;
	for (int i = 100; i < 500; i++) {
		RecordPointer one_record(i, i);
		if (!tree.Insert(i, one_record)) {
			test1_success = false;
		}
	}

	if (!test1_success || tree.IsEmpty()) {
		cout << "ERROR: Insert() test fail!" << endl;
	}
	else {
		cout << "PASS: Insert() test passed!" << endl;
	}

	bool test2_success = true;
	for (int i = 100; i < 500; i++) {
		RecordPointer one_record;
		tree.GetValue(i, one_record);
		if (one_record.page_id != i) {
			test2_success = false;
		}
	}

	if (!test2_success || tree.IsEmpty()) {
		cout << "ERROR: GetValue() test fail!" << endl;
	}
	else {
		cout << "PASS: GetValue() test passed!" << endl;
	}

	for (int i = 100; i < 500; i += 4)
		tree.Remove(i);

	RecordPointer temp;
	if (tree.GetValue(100, temp) || tree.GetValue(204, temp)) {
		cout << "ERROR: Remove() test fail!" << endl;
	}
	else {
		cout << "PASS: Remove() test passed!" << endl;
	}

	vector<RecordPointer> records;
	tree.RangeScan(100, 112, records);
	if (records.size() != 12)
	{
		cout << "ERROR: RangeScan() test fail!" << endl;
	}
	else {
		cout << "PASS: RangeScan() test passed!" << endl;
	}

	if (tree.IsEmpty())
	{
		cout << "ERROR: IsEmpty() test fail!" << endl;
	}
	else {
		cout << "PASS: IsEmpty() test passed!" << endl;
	}

	system("pause");
	return 0;
}
