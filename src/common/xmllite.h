

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* Compiler settings for xmllite.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 7.00.0555 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#ifndef __GNUC__
#pragma warning( disable: 4049 )  /* more than 64k source lines */
#endif
#ifndef __in
	#define __in
#endif
#ifndef __out
	#define __out
#endif
#ifndef __inout
	#define __inout
#endif
#ifndef __in_opt
	#define __in_opt
#endif
#ifndef __out_opt
	#define __out_opt
#endif
#ifndef __out_ecount_part
	#define __out_ecount_part(a,b)
#endif
#ifndef __in_ecount_opt
	#define __in_ecount_opt(a)
#endif


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

/* verify that the <rpcsal.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCSAL_H_VERSION__
#define __REQUIRED_RPCSAL_H_VERSION__ 100
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __xmllite_h__
#define __xmllite_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IXmlReader_FWD_DEFINED__
#define __IXmlReader_FWD_DEFINED__
typedef interface IXmlReader IXmlReader;
#endif 	/* __IXmlReader_FWD_DEFINED__ */


#ifndef __IXmlResolver_FWD_DEFINED__
#define __IXmlResolver_FWD_DEFINED__
typedef interface IXmlResolver IXmlResolver;
#endif 	/* __IXmlResolver_FWD_DEFINED__ */


#ifndef __IXmlWriter_FWD_DEFINED__
#define __IXmlWriter_FWD_DEFINED__
typedef interface IXmlWriter IXmlWriter;
#endif 	/* __IXmlWriter_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "objidl.h"
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_xmllite_0000_0000 */
/* [local] */ 

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//--------------------------------------------------------------------------



typedef IUnknown IXmlReaderInput;
typedef IUnknown IXmlWriterOutput;
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// XmlReader Constructors
STDAPI CreateXmlReader(__in REFIID riid,
    __out void ** ppvObject,
    __in_opt IMalloc * pMalloc);
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// XmlReaderInput Constructors
STDAPI CreateXmlReaderInputWithEncodingCodePage(
    __in IUnknown * pInputStream,
    __in_opt IMalloc * pMalloc,
    __in UINT nEncodingCodePage,
    __in BOOL fEncodingHint,
    __in_opt LPCWSTR pwszBaseUri,
    __out IXmlReaderInput ** ppInput);
STDAPI CreateXmlReaderInputWithEncodingName(
    __in IUnknown * pInputStream,
    __in_opt IMalloc * pMalloc,
    __in LPCWSTR pwszEncodingName,
    __in BOOL fEncodingHint,
    __in_opt LPCWSTR pwszBaseUri,
    __out IXmlReaderInput ** ppInput);
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// XmlWriter Constructors
STDAPI CreateXmlWriter(__in REFIID riid,
    __out void ** ppvObject,
    __in_opt IMalloc * pMalloc);
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// XmlWriterOutput Constructors
STDAPI CreateXmlWriterOutputWithEncodingCodePage(
    __in IUnknown *pOutputStream,
    __in_opt IMalloc *pMalloc,
    __in UINT nEncodingCodePage,
    __out IXmlWriterOutput **ppOutput);
STDAPI CreateXmlWriterOutputWithEncodingName(
    __in IUnknown *pOutputStream,
    __in_opt IMalloc *pMalloc,
    __in LPCWSTR pwszEncodingName,
    __out IXmlWriterOutput **ppOutput);


extern RPC_IF_HANDLE __MIDL_itf_xmllite_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_xmllite_0000_0000_v0_0_s_ifspec;


#ifndef __XmlLite_LIBRARY_DEFINED__
#define __XmlLite_LIBRARY_DEFINED__

/* library XmlLite */
/* [version][lcid][helpstring][uuid] */ 

typedef 
enum XmlNodeType
    {	XmlNodeType_None	= 0,
	XmlNodeType_Element	= 1,
	XmlNodeType_Attribute	= 2,
	XmlNodeType_Text	= 3,
	XmlNodeType_CDATA	= 4,
	XmlNodeType_ProcessingInstruction	= 7,
	XmlNodeType_Comment	= 8,
	XmlNodeType_DocumentType	= 10,
	XmlNodeType_Whitespace	= 13,
	XmlNodeType_EndElement	= 15,
	XmlNodeType_XmlDeclaration	= 17,
	_XmlNodeType_Last	= 17
    } 	XmlNodeType;

typedef 
enum XmlConformanceLevel
    {	XmlConformanceLevel_Auto	= 0,
	XmlConformanceLevel_Fragment	= 1,
	XmlConformanceLevel_Document	= 2,
	_XmlConformanceLevel_Last	= 2
    } 	XmlConformanceLevel;

typedef 
enum DtdProcessing
    {	DtdProcessing_Prohibit	= 0,
	DtdProcessing_Parse	= ( DtdProcessing_Prohibit + 1 ) ,
	_DtdProcessing_Last	= DtdProcessing_Parse
    } 	DtdProcessing;

typedef 
enum XmlReadState
    {	XmlReadState_Initial	= 0,
	XmlReadState_Interactive	= 1,
	XmlReadState_Error	= 2,
	XmlReadState_EndOfFile	= 3,
	XmlReadState_Closed	= 4
    } 	XmlReadState;

typedef 
enum XmlReaderProperty
    {	XmlReaderProperty_MultiLanguage	= 0,
	XmlReaderProperty_ConformanceLevel	= ( XmlReaderProperty_MultiLanguage + 1 ) ,
	XmlReaderProperty_RandomAccess	= ( XmlReaderProperty_ConformanceLevel + 1 ) ,
	XmlReaderProperty_XmlResolver	= ( XmlReaderProperty_RandomAccess + 1 ) ,
	XmlReaderProperty_DtdProcessing	= ( XmlReaderProperty_XmlResolver + 1 ) ,
	XmlReaderProperty_ReadState	= ( XmlReaderProperty_DtdProcessing + 1 ) ,
	XmlReaderProperty_MaxElementDepth	= ( XmlReaderProperty_ReadState + 1 ) ,
	XmlReaderProperty_MaxEntityExpansion	= ( XmlReaderProperty_MaxElementDepth + 1 ) ,
	_XmlReaderProperty_Last	= XmlReaderProperty_MaxEntityExpansion
    } 	XmlReaderProperty;

typedef 
enum XmlError
    {	MX_E_MX	= 0xc00cee00,
	MX_E_INPUTEND	= ( MX_E_MX + 1 ) ,
	MX_E_ENCODING	= ( MX_E_INPUTEND + 1 ) ,
	MX_E_ENCODINGSWITCH	= ( MX_E_ENCODING + 1 ) ,
	MX_E_ENCODINGSIGNATURE	= ( MX_E_ENCODINGSWITCH + 1 ) ,
	WC_E_WC	= 0xc00cee20,
	WC_E_WHITESPACE	= ( WC_E_WC + 1 ) ,
	WC_E_SEMICOLON	= ( WC_E_WHITESPACE + 1 ) ,
	WC_E_GREATERTHAN	= ( WC_E_SEMICOLON + 1 ) ,
	WC_E_QUOTE	= ( WC_E_GREATERTHAN + 1 ) ,
	WC_E_EQUAL	= ( WC_E_QUOTE + 1 ) ,
	WC_E_LESSTHAN	= ( WC_E_EQUAL + 1 ) ,
	WC_E_HEXDIGIT	= ( WC_E_LESSTHAN + 1 ) ,
	WC_E_DIGIT	= ( WC_E_HEXDIGIT + 1 ) ,
	WC_E_LEFTBRACKET	= ( WC_E_DIGIT + 1 ) ,
	WC_E_LEFTPAREN	= ( WC_E_LEFTBRACKET + 1 ) ,
	WC_E_XMLCHARACTER	= ( WC_E_LEFTPAREN + 1 ) ,
	WC_E_NAMECHARACTER	= ( WC_E_XMLCHARACTER + 1 ) ,
	WC_E_SYNTAX	= ( WC_E_NAMECHARACTER + 1 ) ,
	WC_E_CDSECT	= ( WC_E_SYNTAX + 1 ) ,
	WC_E_COMMENT	= ( WC_E_CDSECT + 1 ) ,
	WC_E_CONDSECT	= ( WC_E_COMMENT + 1 ) ,
	WC_E_DECLATTLIST	= ( WC_E_CONDSECT + 1 ) ,
	WC_E_DECLDOCTYPE	= ( WC_E_DECLATTLIST + 1 ) ,
	WC_E_DECLELEMENT	= ( WC_E_DECLDOCTYPE + 1 ) ,
	WC_E_DECLENTITY	= ( WC_E_DECLELEMENT + 1 ) ,
	WC_E_DECLNOTATION	= ( WC_E_DECLENTITY + 1 ) ,
	WC_E_NDATA	= ( WC_E_DECLNOTATION + 1 ) ,
	WC_E_PUBLIC	= ( WC_E_NDATA + 1 ) ,
	WC_E_SYSTEM	= ( WC_E_PUBLIC + 1 ) ,
	WC_E_NAME	= ( WC_E_SYSTEM + 1 ) ,
	WC_E_ROOTELEMENT	= ( WC_E_NAME + 1 ) ,
	WC_E_ELEMENTMATCH	= ( WC_E_ROOTELEMENT + 1 ) ,
	WC_E_UNIQUEATTRIBUTE	= ( WC_E_ELEMENTMATCH + 1 ) ,
	WC_E_TEXTXMLDECL	= ( WC_E_UNIQUEATTRIBUTE + 1 ) ,
	WC_E_LEADINGXML	= ( WC_E_TEXTXMLDECL + 1 ) ,
	WC_E_TEXTDECL	= ( WC_E_LEADINGXML + 1 ) ,
	WC_E_XMLDECL	= ( WC_E_TEXTDECL + 1 ) ,
	WC_E_ENCNAME	= ( WC_E_XMLDECL + 1 ) ,
	WC_E_PUBLICID	= ( WC_E_ENCNAME + 1 ) ,
	WC_E_PESINTERNALSUBSET	= ( WC_E_PUBLICID + 1 ) ,
	WC_E_PESBETWEENDECLS	= ( WC_E_PESINTERNALSUBSET + 1 ) ,
	WC_E_NORECURSION	= ( WC_E_PESBETWEENDECLS + 1 ) ,
	WC_E_ENTITYCONTENT	= ( WC_E_NORECURSION + 1 ) ,
	WC_E_UNDECLAREDENTITY	= ( WC_E_ENTITYCONTENT + 1 ) ,
	WC_E_PARSEDENTITY	= ( WC_E_UNDECLAREDENTITY + 1 ) ,
	WC_E_NOEXTERNALENTITYREF	= ( WC_E_PARSEDENTITY + 1 ) ,
	WC_E_PI	= ( WC_E_NOEXTERNALENTITYREF + 1 ) ,
	WC_E_SYSTEMID	= ( WC_E_PI + 1 ) ,
	WC_E_QUESTIONMARK	= ( WC_E_SYSTEMID + 1 ) ,
	WC_E_CDSECTEND	= ( WC_E_QUESTIONMARK + 1 ) ,
	WC_E_MOREDATA	= ( WC_E_CDSECTEND + 1 ) ,
	WC_E_DTDPROHIBITED	= ( WC_E_MOREDATA + 1 ) ,
	WC_E_INVALIDXMLSPACE	= ( WC_E_DTDPROHIBITED + 1 ) ,
	NC_E_NC	= 0xc00cee60,
	NC_E_QNAMECHARACTER	= ( NC_E_NC + 1 ) ,
	NC_E_QNAMECOLON	= ( NC_E_QNAMECHARACTER + 1 ) ,
	NC_E_NAMECOLON	= ( NC_E_QNAMECOLON + 1 ) ,
	NC_E_DECLAREDPREFIX	= ( NC_E_NAMECOLON + 1 ) ,
	NC_E_UNDECLAREDPREFIX	= ( NC_E_DECLAREDPREFIX + 1 ) ,
	NC_E_EMPTYURI	= ( NC_E_UNDECLAREDPREFIX + 1 ) ,
	NC_E_XMLPREFIXRESERVED	= ( NC_E_EMPTYURI + 1 ) ,
	NC_E_XMLNSPREFIXRESERVED	= ( NC_E_XMLPREFIXRESERVED + 1 ) ,
	NC_E_XMLURIRESERVED	= ( NC_E_XMLNSPREFIXRESERVED + 1 ) ,
	NC_E_XMLNSURIRESERVED	= ( NC_E_XMLURIRESERVED + 1 ) ,
	SC_E_SC	= 0xc00cee80,
	SC_E_MAXELEMENTDEPTH	= ( SC_E_SC + 1 ) ,
	SC_E_MAXENTITYEXPANSION	= ( SC_E_MAXELEMENTDEPTH + 1 ) ,
	WR_E_WR	= 0xc00cef00,
	WR_E_NONWHITESPACE	= ( WR_E_WR + 1 ) ,
	WR_E_NSPREFIXDECLARED	= ( WR_E_NONWHITESPACE + 1 ) ,
	WR_E_NSPREFIXWITHEMPTYNSURI	= ( WR_E_NSPREFIXDECLARED + 1 ) ,
	WR_E_DUPLICATEATTRIBUTE	= ( WR_E_NSPREFIXWITHEMPTYNSURI + 1 ) ,
	WR_E_XMLNSPREFIXDECLARATION	= ( WR_E_DUPLICATEATTRIBUTE + 1 ) ,
	WR_E_XMLPREFIXDECLARATION	= ( WR_E_XMLNSPREFIXDECLARATION + 1 ) ,
	WR_E_XMLURIDECLARATION	= ( WR_E_XMLPREFIXDECLARATION + 1 ) ,
	WR_E_XMLNSURIDECLARATION	= ( WR_E_XMLURIDECLARATION + 1 ) ,
	WR_E_NAMESPACEUNDECLARED	= ( WR_E_XMLNSURIDECLARATION + 1 ) ,
	WR_E_INVALIDXMLSPACE	= ( WR_E_NAMESPACEUNDECLARED + 1 ) ,
	WR_E_INVALIDACTION	= ( WR_E_INVALIDXMLSPACE + 1 ) ,
	WR_E_INVALIDSURROGATEPAIR	= ( WR_E_INVALIDACTION + 1 ) ,
	XML_E_INVALID_DECIMAL	= 0xc00ce01d,
	XML_E_INVALID_HEXIDECIMAL	= 0xc00ce01e,
	XML_E_INVALID_UNICODE	= 0xc00ce01f,
	XML_E_INVALIDENCODING	= 0xc00ce06e
    } 	XmlError;

typedef 
enum XmlStandalone
    {	XmlStandalone_Omit	= 0,
	XmlStandalone_Yes	= 1,
	XmlStandalone_No	= 2,
	_XmlStandalone_Last	= 2
    } 	XmlStandalone;

typedef 
enum XmlWriterProperty
    {	XmlWriterProperty_MultiLanguage	= 0,
	XmlWriterProperty_Indent	= ( XmlWriterProperty_MultiLanguage + 1 ) ,
	XmlWriterProperty_ByteOrderMark	= ( XmlWriterProperty_Indent + 1 ) ,
	XmlWriterProperty_OmitXmlDeclaration	= ( XmlWriterProperty_ByteOrderMark + 1 ) ,
	XmlWriterProperty_ConformanceLevel	= ( XmlWriterProperty_OmitXmlDeclaration + 1 ) ,
	_XmlWriterProperty_Last	= XmlWriterProperty_OmitXmlDeclaration
    } 	XmlWriterProperty;


EXTERN_C const IID LIBID_XmlLite;

#ifndef __IXmlReader_INTERFACE_DEFINED__
#define __IXmlReader_INTERFACE_DEFINED__

/* interface IXmlReader */
/* [unique][uuid][local][object][hidden] */ 


EXTERN_C const IID IID_IXmlReader;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7279FC81-709D-4095-B63D-69FE4B0D9030")
    IXmlReader : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetInput( 
            /* [annotation] */ 
            __in_opt  IUnknown *pInput) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [annotation] */ 
            __in  UINT nProperty,
            /* [annotation] */ 
            __out  LONG_PTR *ppValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [annotation] */ 
            __in  UINT nProperty,
            /* [annotation] */ 
            __in_opt  LONG_PTR pValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Read( 
            /* [annotation] */ 
            __out_opt  XmlNodeType *pNodeType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNodeType( 
            /* [annotation] */ 
            __out  XmlNodeType *pNodeType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MoveToFirstAttribute( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MoveToNextAttribute( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MoveToAttributeByName( 
            /* [annotation] */ 
            __in  LPCWSTR pwszLocalName,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszNamespaceUri) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MoveToElement( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetQualifiedName( 
            /* [annotation] */ 
            __out  LPCWSTR *ppwszQualifiedName,
            /* [annotation] */ 
            __out_opt  UINT *pcwchQualifiedName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNamespaceUri( 
            /* [annotation] */ 
            __out  LPCWSTR *ppwszNamespaceUri,
            /* [annotation] */ 
            __out_opt  UINT *pcwchNamespaceUri) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLocalName( 
            /* [annotation] */ 
            __out  LPCWSTR *ppwszLocalName,
            /* [annotation] */ 
            __out_opt  UINT *pcwchLocalName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPrefix( 
            /* [annotation] */ 
            __out  LPCWSTR *ppwszPrefix,
            /* [annotation] */ 
            __out_opt  UINT *pcwchPrefix) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [annotation] */ 
            __out  LPCWSTR *ppwszValue,
            /* [annotation] */ 
            __out_opt  UINT *pcwchValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadValueChunk( 
            /* [annotation] */ 
            __out_ecount_part(cwchChunkSize, *pcwchRead)  WCHAR *pwchBuffer,
            /* [annotation] */ 
            __in  UINT cwchChunkSize,
            /* [annotation] */ 
            __inout  UINT *pcwchRead) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBaseUri( 
            /* [annotation] */ 
            __out  LPCWSTR *ppwszBaseUri,
            /* [annotation] */ 
            __out_opt  UINT *pcwchBaseUri) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsDefault( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsEmptyElement( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLineNumber( 
            /* [annotation] */ 
            __out  UINT *pnLineNumber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLinePosition( 
            /* [annotation] */ 
            __out  UINT *pnLinePosition) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttributeCount( 
            /* [annotation] */ 
            __out  UINT *pnAttributeCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDepth( 
            /* [annotation] */ 
            __out  UINT *pnDepth) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsEOF( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IXmlReaderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IXmlReader * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IXmlReader * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IXmlReader * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetInput )( 
            IXmlReader * This,
            /* [annotation] */ 
            __in_opt  IUnknown *pInput);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IXmlReader * This,
            /* [annotation] */ 
            __in  UINT nProperty,
            /* [annotation] */ 
            __out  LONG_PTR *ppValue);
        
        HRESULT ( STDMETHODCALLTYPE *SetProperty )( 
            IXmlReader * This,
            /* [annotation] */ 
            __in  UINT nProperty,
            /* [annotation] */ 
            __in_opt  LONG_PTR pValue);
        
        HRESULT ( STDMETHODCALLTYPE *Read )( 
            IXmlReader * This,
            /* [annotation] */ 
            __out_opt  XmlNodeType *pNodeType);
        
        HRESULT ( STDMETHODCALLTYPE *GetNodeType )( 
            IXmlReader * This,
            /* [annotation] */ 
            __out  XmlNodeType *pNodeType);
        
        HRESULT ( STDMETHODCALLTYPE *MoveToFirstAttribute )( 
            IXmlReader * This);
        
        HRESULT ( STDMETHODCALLTYPE *MoveToNextAttribute )( 
            IXmlReader * This);
        
        HRESULT ( STDMETHODCALLTYPE *MoveToAttributeByName )( 
            IXmlReader * This,
            /* [annotation] */ 
            __in  LPCWSTR pwszLocalName,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszNamespaceUri);
        
        HRESULT ( STDMETHODCALLTYPE *MoveToElement )( 
            IXmlReader * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetQualifiedName )( 
            IXmlReader * This,
            /* [annotation] */ 
            __out  LPCWSTR *ppwszQualifiedName,
            /* [annotation] */ 
            __out_opt  UINT *pcwchQualifiedName);
        
        HRESULT ( STDMETHODCALLTYPE *GetNamespaceUri )( 
            IXmlReader * This,
            /* [annotation] */ 
            __out  LPCWSTR *ppwszNamespaceUri,
            /* [annotation] */ 
            __out_opt  UINT *pcwchNamespaceUri);
        
        HRESULT ( STDMETHODCALLTYPE *GetLocalName )( 
            IXmlReader * This,
            /* [annotation] */ 
            __out  LPCWSTR *ppwszLocalName,
            /* [annotation] */ 
            __out_opt  UINT *pcwchLocalName);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrefix )( 
            IXmlReader * This,
            /* [annotation] */ 
            __out  LPCWSTR *ppwszPrefix,
            /* [annotation] */ 
            __out_opt  UINT *pcwchPrefix);
        
        HRESULT ( STDMETHODCALLTYPE *GetValue )( 
            IXmlReader * This,
            /* [annotation] */ 
            __out  LPCWSTR *ppwszValue,
            /* [annotation] */ 
            __out_opt  UINT *pcwchValue);
        
        HRESULT ( STDMETHODCALLTYPE *ReadValueChunk )( 
            IXmlReader * This,
            /* [annotation] */ 
            __out_ecount_part(cwchChunkSize, *pcwchRead)  WCHAR *pwchBuffer,
            /* [annotation] */ 
            __in  UINT cwchChunkSize,
            /* [annotation] */ 
            __inout  UINT *pcwchRead);
        
        HRESULT ( STDMETHODCALLTYPE *GetBaseUri )( 
            IXmlReader * This,
            /* [annotation] */ 
            __out  LPCWSTR *ppwszBaseUri,
            /* [annotation] */ 
            __out_opt  UINT *pcwchBaseUri);
        
        BOOL ( STDMETHODCALLTYPE *IsDefault )( 
            IXmlReader * This);
        
        BOOL ( STDMETHODCALLTYPE *IsEmptyElement )( 
            IXmlReader * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetLineNumber )( 
            IXmlReader * This,
            /* [annotation] */ 
            __out  UINT *pnLineNumber);
        
        HRESULT ( STDMETHODCALLTYPE *GetLinePosition )( 
            IXmlReader * This,
            /* [annotation] */ 
            __out  UINT *pnLinePosition);
        
        HRESULT ( STDMETHODCALLTYPE *GetAttributeCount )( 
            IXmlReader * This,
            /* [annotation] */ 
            __out  UINT *pnAttributeCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetDepth )( 
            IXmlReader * This,
            /* [annotation] */ 
            __out  UINT *pnDepth);
        
        BOOL ( STDMETHODCALLTYPE *IsEOF )( 
            IXmlReader * This);
        
        END_INTERFACE
    } IXmlReaderVtbl;

    interface IXmlReader
    {
        CONST_VTBL struct IXmlReaderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXmlReader_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IXmlReader_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IXmlReader_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IXmlReader_SetInput(This,pInput)	\
    ( (This)->lpVtbl -> SetInput(This,pInput) ) 

#define IXmlReader_GetProperty(This,nProperty,ppValue)	\
    ( (This)->lpVtbl -> GetProperty(This,nProperty,ppValue) ) 

#define IXmlReader_SetProperty(This,nProperty,pValue)	\
    ( (This)->lpVtbl -> SetProperty(This,nProperty,pValue) ) 

#define IXmlReader_Read(This,pNodeType)	\
    ( (This)->lpVtbl -> Read(This,pNodeType) ) 

#define IXmlReader_GetNodeType(This,pNodeType)	\
    ( (This)->lpVtbl -> GetNodeType(This,pNodeType) ) 

#define IXmlReader_MoveToFirstAttribute(This)	\
    ( (This)->lpVtbl -> MoveToFirstAttribute(This) ) 

#define IXmlReader_MoveToNextAttribute(This)	\
    ( (This)->lpVtbl -> MoveToNextAttribute(This) ) 

#define IXmlReader_MoveToAttributeByName(This,pwszLocalName,pwszNamespaceUri)	\
    ( (This)->lpVtbl -> MoveToAttributeByName(This,pwszLocalName,pwszNamespaceUri) ) 

#define IXmlReader_MoveToElement(This)	\
    ( (This)->lpVtbl -> MoveToElement(This) ) 

#define IXmlReader_GetQualifiedName(This,ppwszQualifiedName,pcwchQualifiedName)	\
    ( (This)->lpVtbl -> GetQualifiedName(This,ppwszQualifiedName,pcwchQualifiedName) ) 

#define IXmlReader_GetNamespaceUri(This,ppwszNamespaceUri,pcwchNamespaceUri)	\
    ( (This)->lpVtbl -> GetNamespaceUri(This,ppwszNamespaceUri,pcwchNamespaceUri) ) 

#define IXmlReader_GetLocalName(This,ppwszLocalName,pcwchLocalName)	\
    ( (This)->lpVtbl -> GetLocalName(This,ppwszLocalName,pcwchLocalName) ) 

#define IXmlReader_GetPrefix(This,ppwszPrefix,pcwchPrefix)	\
    ( (This)->lpVtbl -> GetPrefix(This,ppwszPrefix,pcwchPrefix) ) 

#define IXmlReader_GetValue(This,ppwszValue,pcwchValue)	\
    ( (This)->lpVtbl -> GetValue(This,ppwszValue,pcwchValue) ) 

#define IXmlReader_ReadValueChunk(This,pwchBuffer,cwchChunkSize,pcwchRead)	\
    ( (This)->lpVtbl -> ReadValueChunk(This,pwchBuffer,cwchChunkSize,pcwchRead) ) 

#define IXmlReader_GetBaseUri(This,ppwszBaseUri,pcwchBaseUri)	\
    ( (This)->lpVtbl -> GetBaseUri(This,ppwszBaseUri,pcwchBaseUri) ) 

#define IXmlReader_IsDefault(This)	\
    ( (This)->lpVtbl -> IsDefault(This) ) 

#define IXmlReader_IsEmptyElement(This)	\
    ( (This)->lpVtbl -> IsEmptyElement(This) ) 

#define IXmlReader_GetLineNumber(This,pnLineNumber)	\
    ( (This)->lpVtbl -> GetLineNumber(This,pnLineNumber) ) 

#define IXmlReader_GetLinePosition(This,pnLinePosition)	\
    ( (This)->lpVtbl -> GetLinePosition(This,pnLinePosition) ) 

#define IXmlReader_GetAttributeCount(This,pnAttributeCount)	\
    ( (This)->lpVtbl -> GetAttributeCount(This,pnAttributeCount) ) 

#define IXmlReader_GetDepth(This,pnDepth)	\
    ( (This)->lpVtbl -> GetDepth(This,pnDepth) ) 

#define IXmlReader_IsEOF(This)	\
    ( (This)->lpVtbl -> IsEOF(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IXmlReader_INTERFACE_DEFINED__ */


#ifndef __IXmlResolver_INTERFACE_DEFINED__
#define __IXmlResolver_INTERFACE_DEFINED__

/* interface IXmlResolver */
/* [unique][uuid][local][object][hidden] */ 


EXTERN_C const IID IID_IXmlResolver;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7279FC82-709D-4095-B63D-69FE4B0D9030")
    IXmlResolver : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ResolveUri( 
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszBaseUri,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszPublicIdentifier,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszSystemIdentifier,
            /* [annotation] */ 
            __out  IUnknown **ppResolvedInput) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IXmlResolverVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IXmlResolver * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IXmlResolver * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IXmlResolver * This);
        
        HRESULT ( STDMETHODCALLTYPE *ResolveUri )( 
            IXmlResolver * This,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszBaseUri,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszPublicIdentifier,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszSystemIdentifier,
            /* [annotation] */ 
            __out  IUnknown **ppResolvedInput);
        
        END_INTERFACE
    } IXmlResolverVtbl;

    interface IXmlResolver
    {
        CONST_VTBL struct IXmlResolverVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXmlResolver_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IXmlResolver_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IXmlResolver_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IXmlResolver_ResolveUri(This,pwszBaseUri,pwszPublicIdentifier,pwszSystemIdentifier,ppResolvedInput)	\
    ( (This)->lpVtbl -> ResolveUri(This,pwszBaseUri,pwszPublicIdentifier,pwszSystemIdentifier,ppResolvedInput) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IXmlResolver_INTERFACE_DEFINED__ */


#ifndef __IXmlWriter_INTERFACE_DEFINED__
#define __IXmlWriter_INTERFACE_DEFINED__

/* interface IXmlWriter */
/* [unique][uuid][local][object][hidden] */ 


EXTERN_C const IID IID_IXmlWriter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7279FC88-709D-4095-B63D-69FE4B0D9030")
    IXmlWriter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetOutput( 
            /* [annotation] */ 
            __in_opt  IUnknown *pOutput) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [annotation] */ 
            __in  UINT nProperty,
            /* [annotation] */ 
            __out  LONG_PTR *ppValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [annotation] */ 
            __in  UINT nProperty,
            /* [annotation] */ 
            __in_opt  LONG_PTR pValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteAttributes( 
            /* [annotation] */ 
            __in  IXmlReader *pReader,
            /* [annotation] */ 
            __in  BOOL fWriteDefaultAttributes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteAttributeString( 
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszPrefix,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszLocalName,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszNamespaceUri,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteCData( 
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteCharEntity( 
            /* [annotation] */ 
            __in  WCHAR wch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteChars( 
            /* [annotation] */ 
            __in_ecount_opt(cwch)  const WCHAR *pwch,
            /* [annotation] */ 
            __in  UINT cwch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteComment( 
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszComment) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteDocType( 
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszName,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszPublicId,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszSystemId,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszSubset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteElementString( 
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszPrefix,
            /* [annotation] */ 
            __in  LPCWSTR pwszLocalName,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszNamespaceUri,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteEndDocument( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteEndElement( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteEntityRef( 
            /* [annotation] */ 
            __in  LPCWSTR pwszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteFullEndElement( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteName( 
            /* [annotation] */ 
            __in  LPCWSTR pwszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteNmToken( 
            /* [annotation] */ 
            __in  LPCWSTR pwszNmToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteNode( 
            /* [annotation] */ 
            __in  IXmlReader *pReader,
            /* [annotation] */ 
            __in  BOOL fWriteDefaultAttributes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteNodeShallow( 
            /* [annotation] */ 
            __in  IXmlReader *pReader,
            /* [annotation] */ 
            __in  BOOL fWriteDefaultAttributes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteProcessingInstruction( 
            /* [annotation] */ 
            __in  LPCWSTR pwszName,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteQualifiedName( 
            /* [annotation] */ 
            __in  LPCWSTR pwszLocalName,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszNamespaceUri) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteRaw( 
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteRawChars( 
            /* [annotation] */ 
            __in_ecount_opt(cwch)  const WCHAR *pwch,
            /* [annotation] */ 
            __in  UINT cwch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteStartDocument( 
            /* [annotation] */ 
            __in  XmlStandalone standalone) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteStartElement( 
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszPrefix,
            /* [annotation] */ 
            __in  LPCWSTR pwszLocalName,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszNamespaceUri) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteString( 
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteSurrogateCharEntity( 
            /* [annotation] */ 
            __in  WCHAR wchLow,
            /* [annotation] */ 
            __in  WCHAR wchHigh) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteWhitespace( 
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszWhitespace) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Flush( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IXmlWriterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IXmlWriter * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IXmlWriter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IXmlWriter * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetOutput )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in_opt  IUnknown *pOutput);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in  UINT nProperty,
            /* [annotation] */ 
            __out  LONG_PTR *ppValue);
        
        HRESULT ( STDMETHODCALLTYPE *SetProperty )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in  UINT nProperty,
            /* [annotation] */ 
            __in_opt  LONG_PTR pValue);
        
        HRESULT ( STDMETHODCALLTYPE *WriteAttributes )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in  IXmlReader *pReader,
            /* [annotation] */ 
            __in  BOOL fWriteDefaultAttributes);
        
        HRESULT ( STDMETHODCALLTYPE *WriteAttributeString )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszPrefix,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszLocalName,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszNamespaceUri,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszValue);
        
        HRESULT ( STDMETHODCALLTYPE *WriteCData )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszText);
        
        HRESULT ( STDMETHODCALLTYPE *WriteCharEntity )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in  WCHAR wch);
        
        HRESULT ( STDMETHODCALLTYPE *WriteChars )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in_ecount_opt(cwch)  const WCHAR *pwch,
            /* [annotation] */ 
            __in  UINT cwch);
        
        HRESULT ( STDMETHODCALLTYPE *WriteComment )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszComment);
        
        HRESULT ( STDMETHODCALLTYPE *WriteDocType )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszName,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszPublicId,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszSystemId,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszSubset);
        
        HRESULT ( STDMETHODCALLTYPE *WriteElementString )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszPrefix,
            /* [annotation] */ 
            __in  LPCWSTR pwszLocalName,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszNamespaceUri,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszValue);
        
        HRESULT ( STDMETHODCALLTYPE *WriteEndDocument )( 
            IXmlWriter * This);
        
        HRESULT ( STDMETHODCALLTYPE *WriteEndElement )( 
            IXmlWriter * This);
        
        HRESULT ( STDMETHODCALLTYPE *WriteEntityRef )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in  LPCWSTR pwszName);
        
        HRESULT ( STDMETHODCALLTYPE *WriteFullEndElement )( 
            IXmlWriter * This);
        
        HRESULT ( STDMETHODCALLTYPE *WriteName )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in  LPCWSTR pwszName);
        
        HRESULT ( STDMETHODCALLTYPE *WriteNmToken )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in  LPCWSTR pwszNmToken);
        
        HRESULT ( STDMETHODCALLTYPE *WriteNode )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in  IXmlReader *pReader,
            /* [annotation] */ 
            __in  BOOL fWriteDefaultAttributes);
        
        HRESULT ( STDMETHODCALLTYPE *WriteNodeShallow )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in  IXmlReader *pReader,
            /* [annotation] */ 
            __in  BOOL fWriteDefaultAttributes);
        
        HRESULT ( STDMETHODCALLTYPE *WriteProcessingInstruction )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in  LPCWSTR pwszName,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszText);
        
        HRESULT ( STDMETHODCALLTYPE *WriteQualifiedName )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in  LPCWSTR pwszLocalName,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszNamespaceUri);
        
        HRESULT ( STDMETHODCALLTYPE *WriteRaw )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszData);
        
        HRESULT ( STDMETHODCALLTYPE *WriteRawChars )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in_ecount_opt(cwch)  const WCHAR *pwch,
            /* [annotation] */ 
            __in  UINT cwch);
        
        HRESULT ( STDMETHODCALLTYPE *WriteStartDocument )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in  XmlStandalone standalone);
        
        HRESULT ( STDMETHODCALLTYPE *WriteStartElement )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszPrefix,
            /* [annotation] */ 
            __in  LPCWSTR pwszLocalName,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszNamespaceUri);
        
        HRESULT ( STDMETHODCALLTYPE *WriteString )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszText);
        
        HRESULT ( STDMETHODCALLTYPE *WriteSurrogateCharEntity )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in  WCHAR wchLow,
            /* [annotation] */ 
            __in  WCHAR wchHigh);
        
        HRESULT ( STDMETHODCALLTYPE *WriteWhitespace )( 
            IXmlWriter * This,
            /* [annotation] */ 
            __in_opt  LPCWSTR pwszWhitespace);
        
        HRESULT ( STDMETHODCALLTYPE *Flush )( 
            IXmlWriter * This);
        
        END_INTERFACE
    } IXmlWriterVtbl;

    interface IXmlWriter
    {
        CONST_VTBL struct IXmlWriterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXmlWriter_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IXmlWriter_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IXmlWriter_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IXmlWriter_SetOutput(This,pOutput)	\
    ( (This)->lpVtbl -> SetOutput(This,pOutput) ) 

#define IXmlWriter_GetProperty(This,nProperty,ppValue)	\
    ( (This)->lpVtbl -> GetProperty(This,nProperty,ppValue) ) 

#define IXmlWriter_SetProperty(This,nProperty,pValue)	\
    ( (This)->lpVtbl -> SetProperty(This,nProperty,pValue) ) 

#define IXmlWriter_WriteAttributes(This,pReader,fWriteDefaultAttributes)	\
    ( (This)->lpVtbl -> WriteAttributes(This,pReader,fWriteDefaultAttributes) ) 

#define IXmlWriter_WriteAttributeString(This,pwszPrefix,pwszLocalName,pwszNamespaceUri,pwszValue)	\
    ( (This)->lpVtbl -> WriteAttributeString(This,pwszPrefix,pwszLocalName,pwszNamespaceUri,pwszValue) ) 

#define IXmlWriter_WriteCData(This,pwszText)	\
    ( (This)->lpVtbl -> WriteCData(This,pwszText) ) 

#define IXmlWriter_WriteCharEntity(This,wch)	\
    ( (This)->lpVtbl -> WriteCharEntity(This,wch) ) 

#define IXmlWriter_WriteChars(This,pwch,cwch)	\
    ( (This)->lpVtbl -> WriteChars(This,pwch,cwch) ) 

#define IXmlWriter_WriteComment(This,pwszComment)	\
    ( (This)->lpVtbl -> WriteComment(This,pwszComment) ) 

#define IXmlWriter_WriteDocType(This,pwszName,pwszPublicId,pwszSystemId,pwszSubset)	\
    ( (This)->lpVtbl -> WriteDocType(This,pwszName,pwszPublicId,pwszSystemId,pwszSubset) ) 

#define IXmlWriter_WriteElementString(This,pwszPrefix,pwszLocalName,pwszNamespaceUri,pwszValue)	\
    ( (This)->lpVtbl -> WriteElementString(This,pwszPrefix,pwszLocalName,pwszNamespaceUri,pwszValue) ) 

#define IXmlWriter_WriteEndDocument(This)	\
    ( (This)->lpVtbl -> WriteEndDocument(This) ) 

#define IXmlWriter_WriteEndElement(This)	\
    ( (This)->lpVtbl -> WriteEndElement(This) ) 

#define IXmlWriter_WriteEntityRef(This,pwszName)	\
    ( (This)->lpVtbl -> WriteEntityRef(This,pwszName) ) 

#define IXmlWriter_WriteFullEndElement(This)	\
    ( (This)->lpVtbl -> WriteFullEndElement(This) ) 

#define IXmlWriter_WriteName(This,pwszName)	\
    ( (This)->lpVtbl -> WriteName(This,pwszName) ) 

#define IXmlWriter_WriteNmToken(This,pwszNmToken)	\
    ( (This)->lpVtbl -> WriteNmToken(This,pwszNmToken) ) 

#define IXmlWriter_WriteNode(This,pReader,fWriteDefaultAttributes)	\
    ( (This)->lpVtbl -> WriteNode(This,pReader,fWriteDefaultAttributes) ) 

#define IXmlWriter_WriteNodeShallow(This,pReader,fWriteDefaultAttributes)	\
    ( (This)->lpVtbl -> WriteNodeShallow(This,pReader,fWriteDefaultAttributes) ) 

#define IXmlWriter_WriteProcessingInstruction(This,pwszName,pwszText)	\
    ( (This)->lpVtbl -> WriteProcessingInstruction(This,pwszName,pwszText) ) 

#define IXmlWriter_WriteQualifiedName(This,pwszLocalName,pwszNamespaceUri)	\
    ( (This)->lpVtbl -> WriteQualifiedName(This,pwszLocalName,pwszNamespaceUri) ) 

#define IXmlWriter_WriteRaw(This,pwszData)	\
    ( (This)->lpVtbl -> WriteRaw(This,pwszData) ) 

#define IXmlWriter_WriteRawChars(This,pwch,cwch)	\
    ( (This)->lpVtbl -> WriteRawChars(This,pwch,cwch) ) 

#define IXmlWriter_WriteStartDocument(This,standalone)	\
    ( (This)->lpVtbl -> WriteStartDocument(This,standalone) ) 

#define IXmlWriter_WriteStartElement(This,pwszPrefix,pwszLocalName,pwszNamespaceUri)	\
    ( (This)->lpVtbl -> WriteStartElement(This,pwszPrefix,pwszLocalName,pwszNamespaceUri) ) 

#define IXmlWriter_WriteString(This,pwszText)	\
    ( (This)->lpVtbl -> WriteString(This,pwszText) ) 

#define IXmlWriter_WriteSurrogateCharEntity(This,wchLow,wchHigh)	\
    ( (This)->lpVtbl -> WriteSurrogateCharEntity(This,wchLow,wchHigh) ) 

#define IXmlWriter_WriteWhitespace(This,pwszWhitespace)	\
    ( (This)->lpVtbl -> WriteWhitespace(This,pwszWhitespace) ) 

#define IXmlWriter_Flush(This)	\
    ( (This)->lpVtbl -> Flush(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IXmlWriter_INTERFACE_DEFINED__ */

#endif /* __XmlLite_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_xmllite_0001_0113 */
/* [local] */ 

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IID helpers
#if !defined(__GNUC__)
#define IID_IXmlReader   __uuidof(IXmlReader)
#else
const IID IID_IXmlReader = { 0x7279FC81, 0x709D, 0x4095, {0xB6, 0x3D, 0x69, 0xFE, 0x4B, 0x0D, 0x90, 0x30} };
#endif
#if !defined(__GNUC__) || defined(__MINGW64_VERSION_MAJOR)
#define IID_IXmlWriter   __uuidof(IXmlWriter)
#define IID_IXmlResolver __uuidof(IXmlResolver)
#endif


extern RPC_IF_HANDLE __MIDL_itf_xmllite_0001_0113_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_xmllite_0001_0113_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif



