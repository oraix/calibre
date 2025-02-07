/*
 * Copyright (C) 2020 Kovid Goyal <kovid at kovidgoyal.net>
 *
 * Distributed under terms of the GPL3 license.
 */

#pragma once
#define PY_SSIZE_T_CLEAN
#define UNICODE
#define _UNICODE
#include <Windows.h>
#include <Python.h>
#include <comdef.h>
#include "../cpp_binding.h"

static inline PyObject*
set_error_from_hresult(PyObject *exc_type, const char *file, const int line, const HRESULT hr, const char *prefix="", PyObject *name=NULL) {
    _com_error err(hr);
    PyObject *pmsg = PyUnicode_FromWideChar(err.ErrorMessage(), -1);
    if (name) PyErr_Format(exc_type, "%s:%d:%s:[hr=0x%x wCode=%d] %V: %S", file, line, prefix, hr, err.WCode(), pmsg, "Out of memory", name);
    else PyErr_Format(exc_type, "%s:%d:%s:[hr=0x%x wCode=%d] %V", file, line, prefix, hr, err.WCode(), pmsg, "Out of memory");
    Py_CLEAR(pmsg);
    return NULL;
}
#define error_from_hresult(hr, ...) set_error_from_hresult(PyExc_OSError, __FILE__, __LINE__, hr, __VA_ARGS__)

class scoped_com_initializer {  // {{{
	public:
		scoped_com_initializer() : m_succeded(false), hr(0) {
            hr = CoInitialize(NULL);
            if (SUCCEEDED(hr)) m_succeded = true;
        }
		~scoped_com_initializer() { if (succeeded()) CoUninitialize(); }

		explicit operator bool() const noexcept { return m_succeded; }

		bool succeeded() const noexcept { return m_succeded; }

        PyObject* set_python_error() const noexcept {
            if (hr == RPC_E_CHANGED_MODE) {
                PyErr_SetString(PyExc_OSError, "COM initialization failed as it was already initialized in multi-threaded mode");
            } else {
                _com_error err(hr);
                PyObject *pmsg = PyUnicode_FromWideChar(err.ErrorMessage(), -1);
                PyErr_Format(PyExc_OSError, "COM initialization failed: %V", pmsg, "Out of memory");
            }
            return NULL;
        }

        void detach() noexcept { m_succeded = false; }

	private:
		bool m_succeded;
        HRESULT hr;
		scoped_com_initializer( const scoped_com_initializer & ) ;
		scoped_com_initializer & operator=( const scoped_com_initializer & ) ;
}; // }}}

#define INITIALIZE_COM_IN_FUNCTION scoped_com_initializer com; if (!com) return com.set_python_error();

static inline void co_task_mem_free(void* m) { CoTaskMemFree(m); }
typedef generic_raii<wchar_t*, co_task_mem_free, static_cast<wchar_t*>(NULL)> com_wchar_raii;
static inline void handle_destructor(HANDLE p) { CloseHandle(p); }
typedef generic_raii<HANDLE, handle_destructor, INVALID_HANDLE_VALUE> handle_raii;

struct prop_variant : PROPVARIANT {
	prop_variant(VARTYPE vt=VT_EMPTY) noexcept : PROPVARIANT{} { PropVariantInit(this); this->vt = vt;  }

    ~prop_variant() noexcept { clear(); }

    void clear() noexcept { PropVariantClear(this); }
};
