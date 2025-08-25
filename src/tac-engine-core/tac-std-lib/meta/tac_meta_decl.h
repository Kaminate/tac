// This file allows you to declare a metatype using this minimal header
#pragma once
#define TAC_META_DECL( T ) struct MetaType; const MetaType& GetMetaType( const T& )

