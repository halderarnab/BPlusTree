#include "b_plus_tree.h"

/*
* Helper function to decide whether current b+tree is empty
*/
bool BPlusTree::IsEmpty() const {
	if (root != NULL)
		return false;
	return true;
}

/*****************************************************************************
* SEARCH
*****************************************************************************/
/*
* Return the only value that associated with input key
* This method is used for point query
* @return : true means key exists
*/
bool BPlusTree::GetValue(const KeyType &key, RecordPointer &result) {
	if (IsEmpty()) {
		return false;
	}
	Node *c = root;
	int pi; //pointer index
	while (c->is_leaf != true) {
		pi = -1;
		InternalNode *in = (InternalNode*)c;
		//Find smallest number i such that key <= in[i] and store it in pi.
		for (int i = 0; i < in->key_num; i++) {
			if (key <= in->keys[i]) {
				pi = i;
				break;
			}
		}
		if (pi == -1) {
			//last not null pointer should be children[key_num], i.e, if no of keys is 2, no of children pointers should be 3.
			c = in->children[in->key_num];
		}
		else if (key == in->keys[pi]) {
			//Value is equal to the key.
			c = in->children[pi + 1];
		}
		else {
			//Value is less than the key.
			c = in->children[pi];
		}
	}

	//c is a leaf node.
	LeafNode *leaf = (LeafNode*)c;
	for (int i = 0; i < leaf->key_num; i++) {
		//printf("%d\n", c->keys[i]);
		if (leaf->keys[i] == key) {
			//result = leaf->pointers[i];
			result.page_id = leaf->pointers[i].page_id;
			return true;
		}
	}
	return false;
}

/*****************************************************************************
* INSERTION
*****************************************************************************/
/*
* Insert constant key & value pair into b+ tree
* If current tree is empty, start new tree, otherwise insert into leaf Node.
* @return: since we only support unique key, if user try to insert duplicate
* keys return false, otherwise return true.
*/
bool BPlusTree::Insert(const KeyType &key, const RecordPointer &value) {
	//printf("Insert key1: %d\n", key);
	//Check if duplicate value is being inserted.
	RecordPointer one_record;
	if (GetValue(key, one_record)) {
		return false;
	}
	//If tree is empty, create root node that is also a leaf.
	if (IsEmpty()) {
		LeafNode *r = new LeafNode();
		r->keys[0] = key;
		r->key_num = 1;
		r->pointers[0] = value;
		root = r;
		return true;
	}
	Node *c = root;
	InternalNode *parent;
	int pi; //pointer index
	while (c->is_leaf != true) {
		parent = (InternalNode*)c;
		pi = -1;
		InternalNode *in = (InternalNode*)c;
		//Find smallest number i such that key <= in[i] and store it in pi.
		for (int i = 0; i < in->key_num; i++) {
			if (key <= in->keys[i]) {
				pi = i;
				break;
			}
		}
		if (pi == -1) {
			//last not null pointer should be children[key_num], i.e, if no of keys is 2, no of children pointers should be 3.
			c = in->children[in->key_num];
		}
		else if (key == in->keys[pi]) {
			//Value is equal to the key.
			c = in->children[pi + 1];
		}
		else {
			//Value is less that the key.
			c = in->children[pi];
		}
	}
	//Leaf node has less than n - 1 keys.
	if (c->key_num < MAX_FANOUT - 1) {
		InsertInLeaf((LeafNode*)c, key, value);
		return true;
	}
	//Leaf node has n - 1 keys and needs to be splitted.
	else {
		//temp key array of size n, to store keys from current leaf node.
		KeyType tk[MAX_FANOUT];
		//temp RecordPointer array of size n, to store RecordPointers from current leaf node.
		RecordPointer trp[MAX_FANOUT];
		for (int i = 0; i < MAX_FANOUT - 1; i++) {
			tk[i] = c->keys[i];
			trp[i] = ((LeafNode*)c)->pointers[i];
		}
		int k = 0;
		//Finding the position of the new key.
		while (key > tk[k] && k < MAX_FANOUT - 1) {
			k++;
		}
		//Shifting keys and pointers to correct position.
		for (int j = MAX_FANOUT - 1; j > k; j--) {
			tk[j] = tk[j - 1];
			trp[j] = trp[j - k];
		}
		//Putting the key and pointer/value in correct position. 
		tk[k] = key;
		trp[k] = value;
		//New leaf node.
		LeafNode *l = new LeafNode();
		//Linking two leaf node's next_leaf and prev_leaf.
		l->next_leaf = ((LeafNode*)c)->next_leaf;
		((LeafNode*)c)->next_leaf = l;
		l->prev_leaf = (LeafNode*)c;
		//Linking leaf->next_leaf node's previous leaf, if it exists.
		if (l->next_leaf != NULL) {
			(l->next_leaf)->prev_leaf = l;
		}
		//Setting the size of current leaf and new leaf nodes.
		c->key_num = MAX_FANOUT / 2; //Size of current leaf.
		l->key_num = MAX_FANOUT - MAX_FANOUT / 2; //Size of new leaf.
												  //Copying elements from tk and tpr (temp key and pointer array) to current and new leaf node.
		for (int i = 0; i < c->key_num; i++) {
			c->keys[i] = tk[i];
			((LeafNode*)c)->pointers[i] = trp[i];
		}
		for (int i = 0, j = MAX_FANOUT / 2; i < l->key_num; i++, j++) {
			l->keys[i] = tk[j];
			l->pointers[i] = trp[j];
		}
		//Function to insert key in parent.
		InsertInParent(l->keys[0], parent, (InternalNode*)l, (LeafNode*)c);
	}
	return true;
}

//Function to insert key in parent.
void BPlusTree::InsertInParent(const KeyType &key, InternalNode *parent, InternalNode *l, Node *c) {
	//printf("InsertInParent key2: %d\n", key);
	//If c is both leaf and root node, need to create a new root node.
	if (c == root) {
		InternalNode *r = new InternalNode();
		r->keys[0] = key;
		r->children[0] = c;
		r->children[1] = l;
		r->key_num = 1;
		root = r;
	}
	else {
		//If parent has less than n children.
		if (parent->key_num < MAX_FANOUT - 1) {
			int k = 0;
			//Finding the position of the key.
			while (key > parent->keys[k] && k < parent->key_num) {
				k++;
			}
			//Shifting keys to correct position.
			for (int j = parent->key_num; j > k; j--)
				parent->keys[j] = parent->keys[j - 1];
			//Shifting children pointers to correct position.
			for (int j = parent->key_num + 1; j > k + 1; j--) {
				parent->children[j] = (InternalNode*)parent->children[j - 1];
			}
			//Putting the key and child pointer in correct position.
			parent->keys[k] = key;
			parent->key_num++;
			parent->children[k + 1] = l;
		}
		//Splitting parent node.
		else {
			//temp key array of size n, to store keys from parent node. 
			KeyType tk[MAX_FANOUT];
			//temp Node array of size n + 1, to store children from parent node.
			Node *tc[MAX_FANOUT + 1];
			//Copying keys from parent to temp key array.
			for (int i = 0; i < MAX_FANOUT - 1; i++) {
				tk[i] = parent->keys[i];
			}
			//Copying children pointers from parent to temp Node array.
			for (int i = 0; i < MAX_FANOUT; i++) {
				tc[i] = parent->children[i];
			}
			int k = 0;
			//Finding the position of the key.
			while (key > tk[k] && k < MAX_FANOUT - 1) {
				k++;
			}
			//Shifting keys to correct position.
			for (int i = MAX_FANOUT - 1; i > k; i--) {
				tk[i] = tk[i - 1];
			}
			//Putting the key in correct position.
			tk[k] = key;
			//Shifting children pointers to correct position.
			for (int i = MAX_FANOUT; i > k + 1; i--) {
				tc[i] = tc[i - 1];
			}
			//Putting the child pointer in correct position.
			tc[k + 1] = (InternalNode*)l;
			InternalNode *p = new InternalNode(); //New internal parent node.
			parent->key_num = MAX_FANOUT / 2; //Set the size of parent node.
			p->key_num = MAX_FANOUT - MAX_FANOUT / 2 - 1; //Set the size of new internal parent node.
			for (int i = 0, j = MAX_FANOUT / 2 + 1; i < p->key_num; i++, j++) {
				p->keys[i] = tk[j];
			}
			for (int i = 0, j = MAX_FANOUT / 2 + 1; i < p->key_num + 1; i++, j++) {
				p->children[i] = (InternalNode*)tc[j];
			}
			InsertInParent(tk[MAX_FANOUT / 2], FindParent(root, parent), p, parent);
		}
	}
}

//Function to find the parent of any node.
InternalNode* BPlusTree::FindParent(Node *root, InternalNode *cursor) {
	InternalNode *parent = NULL;
	InternalNode *tmp = (InternalNode*)root;
	//If temp/root reaches the end of tree.
	if (tmp->is_leaf || tmp->children[0]->is_leaf) {
		return NULL;
	}
	//Traverse all the children of the current node.
	for (int i = 0; i < tmp->key_num + 1; i++) {
		//Set the parent of the child node.
		if (tmp->children[i] == cursor) {
			parent = tmp;
			return parent;
		}
		//Recursively traverse to find child node.
		else {
			parent = FindParent(tmp->children[i], cursor);
			if (parent != NULL) {
				return parent;
			}
		}
	}
	return parent;

}


void BPlusTree::InsertInLeaf(LeafNode *leaf, const KeyType &key, const RecordPointer &value) {
	//Add the new Key at the last empty position.
	leaf->keys[leaf->key_num] = key;
	leaf->pointers[leaf->key_num] = value;
	leaf->key_num++;
	//Sort the array to put the new key and pointer in correct position.
	sort(leaf);
}

//Insertion sort to sort the array and put the new key and pointer in correct position.
void BPlusTree::sort(LeafNode *leaf) {
	int k, j;
	RecordPointer pointer;
	for (int i = 1; i < leaf->key_num; i++)
	{
		k = leaf->keys[i];
		pointer = leaf->pointers[i];
		j = i - 1;
		while (j >= 0 && leaf->keys[j] > k)
		{
			leaf->keys[j + 1] = leaf->keys[j];
			leaf->pointers[j + 1] = leaf->pointers[j];
			j = j - 1;
		}
		leaf->keys[j + 1] = k;
		leaf->pointers[j + 1] = pointer;
	}
}

//Function to print tree level vise
void BPlusTree::PrintTree() {
	int nodeNo = 1;
	//Queue to store nodes.
	queue<Node*> q;
	q.push(root);
	Node *temp;
	while (!q.empty() and q.front() != NULL) {
		printf("---- Elements in Node %d: ----\n", nodeNo);
		temp = q.front();
		q.pop();
		for (int i = 0; i < temp->key_num; i++) {
			printf("%d\t", temp->keys[i]);
		}
		for (int i = 0; i < temp->key_num + 1; i++) {
			if (!((InternalNode*)temp)->is_leaf)
				q.push(((InternalNode*)temp)->children[i]);
		}
		printf("\n");
		nodeNo++;
	}
}

/*****************************************************************************
* REMOVE
*****************************************************************************/
/*
* Delete key & value pair associated with input key
* If current tree is empty, return immdiately.
* If not, User needs to first find the right leaf node as deletion target, then
* delete entry from leaf node. Remember to deal with redistribute or merge if
* necessary.
*/
void BPlusTree::Remove(const KeyType &key) {
	RecordPointer one_record;
	if (IsEmpty()) {
		return;
	}
	else if (!GetValue(key, one_record)) { //Key is not present.
		return;
	}
	Node *c = root;
	int pi; //pointer index
	while (c->is_leaf != true) {
		pi = -1;
		InternalNode *in = (InternalNode*)c;
		//Find smallest number i such that key_start <= in[i] and store it in pi.
		for (int i = 0; i < in->key_num; i++) {
			if (key <= in->keys[i]) {
				pi = i;
				break;
			}
		}
		if (pi == -1) {
			//last not null pointer should be children[key_num], i.e, if no of keys is 2, no of children pointers should be 3.
			c = in->children[in->key_num];
		}
		else if (key == in->keys[pi]) {
			//Value is equal to the key_start.
			c = in->children[pi + 1];
		}
		else {
			//Value is less than the key_start.
			c = in->children[pi];
		}
	}

	//c is a leaf node.
	LeafNode *leaf = (LeafNode*)c;
	//*****NOTE: Below function implementation not complete.*****
	//DeleteEntry(leaf, key);
}

void BPlusTree::DeleteEntry(Node *n, const KeyType &key) {
	//*****NOTE: Function implementation not complete.*****
	LeafNode *c = (LeafNode*)n;
	int k = 0;
	while (key != c->keys[k] && k < c->key_num - 1) {
		k++;
	}
	for (int i = k; i < c->key_num; i++) {
		c->keys[i] = c->keys[i + 1];
		c->pointers[i] = c->pointers[i + 1];
	}
	c->key_num--;
	if (c == root && c->is_leaf == false && c->key_num == 0) {
		root = ((InternalNode*)c)->children[0];
		delete c;
	}
	else if (c->key_num < (MAX_FANOUT - 1) / 2) {
		InternalNode *n1 = FindParent(root, (InternalNode*)c);
		//n1 = n1->children[];
	}
	//Remaning logic yet to be implemented.
}

/*****************************************************************************
* RANGE_SCAN
*****************************************************************************/
/*
* Return the values that within the given key range
* First find the node large or equal to the key_start, then traverse the leaf
* nodes until meet the key_end position, fetch all the records.
*/
void BPlusTree::RangeScan(const KeyType &key_start, const KeyType &key_end,
	std::vector<RecordPointer> &result) {
	if (IsEmpty()) {
		return;
	}
	Node *c = root;
	int pi; //pointer index
	while (c->is_leaf != true) {
		pi = -1;
		InternalNode *in = (InternalNode*)c;
		//Find smallest number i such that key_start <= in[i] and store it in pi.
		for (int i = 0; i < in->key_num; i++) {
			if (key_start <= in->keys[i]) {
				pi = i;
				break;
			}
		}
		if (pi == -1) {
			//last not null pointer should be children[key_num], i.e, if no of keys is 2, no of children pointers should be 3.
			c = in->children[in->key_num];
		}
		else if (key_start == in->keys[pi]) {
			//Value is equal to the key_start.
			c = in->children[pi + 1];
		}
		else {
			//Value is less than the key_start.
			c = in->children[pi];
		}
	}

	//c is a leaf node.
	LeafNode *leaf = (LeafNode*)c;
	//pi is the least value such that keys[pi] >= key_start.
	pi = -1;
	for (int i = 0; i < leaf->key_num; i++) {
		if (leaf->keys[i] >= key_start) {
			pi = i;
			break;
		}
	}
	if (pi == -1) {
		//To move to next leaf.
		pi = 1 + leaf->key_num;
	}
	bool done = false;
	while (!done) {
		int n = leaf->key_num;
		if (pi <= n && leaf->keys[pi] < key_end) {
			result.push_back(leaf->pointers[pi]);
			pi++;
		}
		else if (pi <= n && leaf->keys[pi] >= key_end) {
			//All the keys have been found.
			done = true;
		}
		else if (pi > n && leaf->next_leaf != NULL) {
			//Move to next leaf.
			leaf = leaf->next_leaf;
			pi = 1;
		}
		else {
			//No more leaves to move to.
			done = true;
		}
	}
}
