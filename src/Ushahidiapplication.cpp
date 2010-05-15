/*
    Copyright (C) 2010 Kasidit Yusuf.

    This file is part of "Ushahidi Symbian Uploader".

    "Ushahidi Symbian Uploader" is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    "Ushahidi Symbian Uploader" is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with "Ushahidi Symbian Uploader".  If not, see <http://www.gnu.org/licenses/>.
*/

// INCLUDE FILES
#include "Ushahididocument.h"
#include "Ushahidiapplication.h"


// -----------------------------------------------------------------------------
// CUshahidiApplication::CreateDocumentL()
// Create an Ushahidi document, and return a pointer to it
// -----------------------------------------------------------------------------
//
CApaDocument* CUshahidiApplication::CreateDocumentL()
    {
    return( static_cast< CApaDocument* >( CUshahidiDocument::NewL( *this ) ) );
    }

// -----------------------------------------------------------------------------
// CUshahidiApplication::AppDllUid()
// Returns application UID
// -----------------------------------------------------------------------------
//
TUid CUshahidiApplication::AppDllUid() const
    {
    return KUidUshahidiApp;
    }

// End of File
