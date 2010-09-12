/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


const HRESULT   E_ALREADY_INIT = HRESULT_FROM_WIN32( ERROR_ALREADY_INITIALIZED );
const HRESULT   E_WRONG_THREAD = HRESULT_FROM_WIN32( ERROR_INVALID_THREAD_ID );
// the alternatives for E_WRONG_THREAD are RPC_E_ATTEMPTED_MULTITHREAD and RPC_E_WRONG_THREAD
const HRESULT   E_TIMEOUT = HRESULT_FROM_WIN32( ERROR_SEM_TIMEOUT );
const HRESULT   E_NOT_FOUND = HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
const HRESULT   E_INSUFFICIENT_BUFFER = HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );