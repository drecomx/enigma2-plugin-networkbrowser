/*
 * showmount.c -- show mount information for an NFS server
 *
 * Copyright (C) 2018 Dream Property GmbH
 *
 * This plugin is licensed under the Creative Commons
 * Attribution-NonCommercial-ShareAlike 3.0 Unported
 * License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to Creative
 * Commons, 559 Nathan Abbott Way, Stanford, California 94305, USA.
 *
 * Alternatively, this plugin may be distributed and executed on hardware which
 * is licensed by Dream Property GmbH.
 *
 * This plugin is NOT free software. It is open source, you are allowed to
 * modify it (if you keep the license), but it may not be commercially
 * distributed other than under the conditions noted above.
 */

#ifdef HAVE_CONFIG_H
#include <enigma2-plugins-config.h>
#endif
#include <netinet/tcp.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <nfsc/libnfs.h>
#include <nfsc/libnfs-raw.h>
#include <nfsc/libnfs-raw-mount.h>

extern void rpc_set_tcp_syncnt(struct rpc_context *rpc, int v);
extern void rpc_set_timeout(struct rpc_context *rpc, int timeout);

struct exports_ctx {
	int done;
	struct exportnode *ex;
};

static void pyexport(PyObject *result, const char *ex_dir, const struct groupnode *ex_groups)
{
	PyObject *dict, *dir, *groups;
	const struct groupnode *gr;

	if (ex_dir == NULL)
		return;

	dict = PyDict_New();
	if (dict == NULL)
		return;

	groups = PyList_New(0);
	if (groups == NULL) {
		Py_DECREF(dict);
		return;
	}

	dir = PyString_FromString(ex_dir);
	PyDict_SetItemString(dict, "dir", dir);
	Py_DECREF(dir);
	PyDict_SetItemString(dict, "groups", groups);
	Py_DECREF(groups);
	PyList_Append(result, dict);
	Py_DECREF(dict);

	for (gr = ex_groups; gr != NULL; gr = gr->gr_next) {
		PyObject *name = PyString_FromString(gr->gr_name);
		PyList_Append(groups, name);
		Py_DECREF(name);
	}
}

static struct groupnode *groupnode_dup(struct groupnode *gr)
{
	struct groupnode *copy;

	copy = calloc(1, sizeof(*copy));
	copy->gr_name = strdup(gr->gr_name);

	return copy;
}

static void groupnode_free(struct groupnode *gr)
{
	free(gr->gr_name);
	free(gr);
}

static struct groupnode *groups_dup(struct groupnode *gr)
{
	struct groupnode *copy, *list = NULL;

	if (gr) {
		copy = list = groupnode_dup(gr);
		while (gr->gr_next) {
			gr = gr->gr_next;
			copy->gr_next = groupnode_dup(gr);
			copy = copy->gr_next;
		}
	}

	return list;
}

static void groups_free(struct groupnode *gr)
{
	struct groupnode *next;

	while (gr) {
		next = gr->gr_next;
		groupnode_free(gr);
		gr = next;
	}
}

static struct exportnode *exportnode_dup(struct exportnode *ex)
{
	struct exportnode *copy;

	copy = calloc(1, sizeof(*copy));
	copy->ex_dir = strdup(ex->ex_dir);
	copy->ex_groups = groups_dup(ex->ex_groups);

	return copy;
}

static void exportnode_free(struct exportnode *ex)
{
	free(ex->ex_dir);
	groups_free(ex->ex_groups);
	free(ex);
}

static struct exportnode *exports_dup(struct exportnode *ex)
{
	struct exportnode *copy, *list = NULL;

	if (ex) {
		copy = list = exportnode_dup(ex);
		while (ex->ex_next) {
			ex = ex->ex_next;
			copy->ex_next = exportnode_dup(ex);
			copy = copy->ex_next;
		}
	}

	return list;
}

static void exports_free(struct exportnode *ex)
{
	struct exportnode *next;

	while (ex) {
		next = ex->ex_next;
		exportnode_free(ex);
		ex = next;
	}
}

static void exports_cb(struct rpc_context *rpc, int status, void *data, void *private_data)
{
	struct exports_ctx *ctx = private_data;
	struct exportnode *ex = *(struct exportnode **)data;

	if (status == 0)
		ctx->ex = exports_dup(ex);

	ctx->done = 1;
}

static PyObject *showmount(PyObject *self, PyObject *args)
{
	PyObject *result;
	char *hostname;
	int ret;
	struct nfs_context *nfs;
	struct rpc_context *rpc;
	struct exportnode *ex = NULL;
	struct exports_ctx ctx = {
		.done = 0,
		.ex = NULL,
	};

	if (!PyArg_ParseTuple(args, "s", &hostname)) {
		PyErr_SetString(PyExc_TypeError, "showmount(node)");
		return NULL;
	}

	result = PyList_New(0);
	if (result == NULL)
		return result;

	rpc = rpc_init_context();
	if (rpc) {
		Py_BEGIN_ALLOW_THREADS;

		ret = mount_getexports_async(rpc, hostname, exports_cb, &ctx);
		if (ret == 0) {
			rpc_set_tcp_syncnt(rpc, 2);
			rpc_set_timeout(rpc, 1000);
			for (;;) {
				struct pollfd pfd = {
					.fd = rpc_get_fd(rpc),
					.events = rpc_which_events(rpc),
					.revents = 0,
				};
				if (poll(&pfd, 1, 1000) < 0)
					break;
				if (rpc_service(rpc, pfd.revents) < 0)
					break;
				if (ctx.done) {
					ex = ctx.ex;
					break;
				}
			}
		}
		rpc_destroy_context(rpc);

		Py_END_ALLOW_THREADS;
	}

	while (ex) {
		pyexport(result, ex->ex_dir, ex->ex_groups);
		ex = ex->ex_next;
	}

	if (ctx.ex) {
		exports_free(ctx.ex);
	} else {
		nfs = nfs_init_context();
		if (nfs) {
			Py_BEGIN_ALLOW_THREADS;

			nfs_set_tcp_syncnt(nfs, 2);
			nfs_set_timeout(nfs, 1000);
			nfs_set_version(nfs, 4);
			ret = nfs_mount(nfs, hostname, "/");
			nfs_destroy_context(nfs);

			Py_END_ALLOW_THREADS;

			if (ret == 0)
				pyexport(result, "/", NULL);
		}
	}

	return result;
}

static PyMethodDef ops[] = {
	{ "showmount", showmount, METH_VARARGS },
	{ NULL, }
};

void initnfsutils(void)
{
	Py_InitModule("nfsutils", ops);
}
