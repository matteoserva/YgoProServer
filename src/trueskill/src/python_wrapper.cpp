#include <Python.h>
#include <vector>
#include "trueskill.h"

static PyObject* _adjust_players(PyObject* self, PyObject* args) {
	PyObject* list_obj;
	PyObject* seq;
	int size, i = 0;

	// get the list from the arguments
	// note: "O" flag means it's a python object
	if (!PyArg_ParseTuple(args, "O", &list_obj)) {
		return NULL;
	}

	// verify that it's actually a list
	seq = PySequence_Fast(list_obj, "expected a list");
	if (!seq) {
		return NULL;
	}

	// get the list size
	size = PySequence_Size(seq);
	if (size < 0) {
		return NULL;
	}

	// create a vector of players to hold our soon-to-be-created player objects
	std::vector<Player*> players;
	PyObject* py_tuple;
	for (i = 0; i < size; ++i) {
		// create a new player object
		Player* p = new Player();

		// get the current item from the list, which happens to be a tuple (mu, sigma, rank)
		py_tuple = PySequence_Fast_GET_ITEM(seq, i);


		// convert the tuple items into their c types
		p->mu = PyFloat_AsDouble(PyTuple_GetItem(py_tuple, 0));
		p->sigma = PyFloat_AsDouble(PyTuple_GetItem(py_tuple, 1));
		p->rank = (int)PyInt_AsLong(PyTuple_GetItem(py_tuple, 2));

		// add the player to the players vector
		players.push_back(p);
	}

	// run trueskill on the players
	TrueSkill ts;
	ts.adjust_players(players);

	// create the result list
	PyObject* result = PyList_New(size);
	for (i = 0; i < size; ++i) {
		// create a tuple and set it's values for the player
		PyObject* py_tuple;

		py_tuple = PyTuple_New(3);
		PyTuple_SetItem(py_tuple, 0, PyFloat_FromDouble(players[i]->mu));
		PyTuple_SetItem(py_tuple, 1, PyFloat_FromDouble(players[i]->sigma));
		PyTuple_SetItem(py_tuple, 2, PyInt_FromLong((long)players[i]->rank));

		// push the tuple onto the list
		PyList_SetItem(result, i, py_tuple);
	}

	// return the list
	return result;
}

static PyMethodDef functions[] = {
	{"adjust_players", _adjust_players, METH_VARARGS},
	{NULL, NULL}
};

PyMODINIT_FUNC init_trueskill(void) {
	Py_InitModule4("_trueskill", functions, "trueskill module", NULL, PYTHON_API_VERSION);
}
