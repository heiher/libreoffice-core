/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <hintids.hxx>
#include <hints.hxx>
#include <svl/ctloptions.hxx>
#include <sfx2/printer.hxx>
#include <sfx2/sfxuno.hxx>
#include <editeng/langitem.hxx>
#include <editeng/lspcitem.hxx>
#include <editeng/lrspitem.hxx>
#include <editeng/ulspitem.hxx>
#include <editeng/brushitem.hxx>
#include <editeng/pgrditem.hxx>
#include <swmodule.hxx>
#include <SwSmartTagMgr.hxx>
#include <doc.hxx>
#include <IDocumentSettingAccess.hxx>
#include <IDocumentDeviceAccess.hxx>
#include <IDocumentFieldsAccess.hxx>
#include "rootfrm.hxx"
#include <pagefrm.hxx>
#include <viewsh.hxx>
#include <pam.hxx>
#include <ndtxt.hxx>
#include <txtatr.hxx>
#include <paratr.hxx>
#include <viewopt.hxx>
#include <dflyobj.hxx>
#include <flyfrm.hxx>
#include <tabfrm.hxx>
#include <frmtool.hxx>
#include <pagedesc.hxx>
#include <tgrditem.hxx>
#include <dbg_lay.hxx>
#include <fmtfld.hxx>
#include <fmtftn.hxx>
#include <txtfld.hxx>
#include <txtftn.hxx>
#include <charatr.hxx>
#include <ftninfo.hxx>
#include <fmtline.hxx>
#include <txtfrm.hxx>
#include <sectfrm.hxx>
#include <itrform2.hxx>
#include <widorp.hxx>
#include <txtcache.hxx>
#include <fntcache.hxx>
#include <SwGrammarMarkUp.hxx>
#include <lineinfo.hxx>
#include <SwPortionHandler.hxx>
#include <dcontact.hxx>
#include <sortedobjs.hxx>
#include <txtflcnt.hxx>
#include <fmtflcnt.hxx>
#include <fmtcntnt.hxx>
#include <numrule.hxx>
#include <swtable.hxx>
#include <fldupde.hxx>
#include <IGrammarContact.hxx>
#include <calbck.hxx>
#include <ftnidx.hxx>

TYPEINIT1( SwTextFrm, SwContentFrm );

/// Switches width and height of the text frame
void SwTextFrm::SwapWidthAndHeight()
{
    if ( ! bIsSwapped )
    {
        const long nPrtOfstX = Prt().Pos().X();
        Prt().Pos().X() = Prt().Pos().Y();
        if( IsVertLR() )
            Prt().Pos().Y() = nPrtOfstX;
        else
            Prt().Pos().Y() = Frm().Width() - ( nPrtOfstX + Prt().Width() );

    }
    else
    {
        const long nPrtOfstY = Prt().Pos().Y();
        Prt().Pos().Y() = Prt().Pos().X();
        if( IsVertLR() )
            Prt().Pos().X() = nPrtOfstY;
        else
            Prt().Pos().X() = Frm().Height() - ( nPrtOfstY + Prt().Height() );
    }

    const long nFrmWidth = Frm().Width();
    Frm().Width( Frm().Height() );
    Frm().Height( nFrmWidth );
    const long nPrtWidth = Prt().Width();
    Prt().Width( Prt().Height() );
    Prt().Height( nPrtWidth );

    bIsSwapped = ! bIsSwapped;
}

/**
 * Calculates the coordinates of a rectangle when switching from
 * horizontal to vertical layout.
 */
void SwTextFrm::SwitchHorizontalToVertical( SwRect& rRect ) const
{
    // calc offset inside frame
    long nOfstX, nOfstY;
    if ( IsVertLR() )
    {
        nOfstX = rRect.Left() - Frm().Left();
        nOfstY = rRect.Top() - Frm().Top();
    }
    else
    {
        nOfstX = rRect.Left() - Frm().Left();
        nOfstY = rRect.Top() + rRect.Height() - Frm().Top();
    }

    const long nWidth = rRect.Width();
    const long nHeight = rRect.Height();

    if ( IsVertLR() )
        rRect.Left(Frm().Left() + nOfstY);
    else
    {
        if ( bIsSwapped )
            rRect.Left( Frm().Left() + Frm().Height() - nOfstY );
        else
            // frame is rotated
            rRect.Left( Frm().Left() + Frm().Width() - nOfstY );
    }

    rRect.Top( Frm().Top() + nOfstX );
    rRect.Width( nHeight );
    rRect.Height( nWidth );
}

/**
 * Calculates the coordinates of a point when switching from
 * horizontal to vertical layout.
 */
void SwTextFrm::SwitchHorizontalToVertical( Point& rPoint ) const
{
    // calc offset inside frame
    const long nOfstX = rPoint.X() - Frm().Left();
    const long nOfstY = rPoint.Y() - Frm().Top();
    if ( IsVertLR() )
        rPoint.X() = Frm().Left() + nOfstY;
    else
    {
        if ( bIsSwapped )
            rPoint.X() = Frm().Left() + Frm().Height() - nOfstY;
        else
            // calc rotated coords
            rPoint.X() = Frm().Left() + Frm().Width() - nOfstY;
    }

    rPoint.Y() = Frm().Top() + nOfstX;
}

/**
 * Calculates the a limit value when switching from
 * horizontal to vertical layout.
 */
long SwTextFrm::SwitchHorizontalToVertical( long nLimit ) const
{
    Point aTmp( 0, nLimit );
    SwitchHorizontalToVertical( aTmp );
    return aTmp.X();
}

/**
 * Calculates the coordinates of a rectangle when switching from
 * vertical to horizontal layout.
 */
void SwTextFrm::SwitchVerticalToHorizontal( SwRect& rRect ) const
{
    long nOfstX;

    // calc offset inside frame
    if ( IsVertLR() )
        nOfstX = rRect.Left() - Frm().Left();
    else
    {
        if ( bIsSwapped )
            nOfstX = Frm().Left() + Frm().Height() - ( rRect.Left() + rRect.Width() );
        else
            nOfstX = Frm().Left() + Frm().Width() - ( rRect.Left() + rRect.Width() );
    }

    const long nOfstY = rRect.Top() - Frm().Top();
    const long nWidth = rRect.Height();
    const long nHeight = rRect.Width();

    // calc rotated coords
    rRect.Left( Frm().Left() + nOfstY );
    rRect.Top( Frm().Top() + nOfstX );
    rRect.Width( nWidth );
    rRect.Height( nHeight );
}

/**
 * Calculates the coordinates of a point when switching from
 * vertical to horizontal layout.
 */
void SwTextFrm::SwitchVerticalToHorizontal( Point& rPoint ) const
{
    long nOfstX;

    // calc offset inside frame
    if ( IsVertLR() )
        nOfstX = rPoint.X() - Frm().Left();
    else
    {
        if ( bIsSwapped )
            nOfstX = Frm().Left() + Frm().Height() - rPoint.X();
        else
            nOfstX = Frm().Left() + Frm().Width() - rPoint.X();
    }

    const long nOfstY = rPoint.Y() - Frm().Top();

    // calc rotated coords
    rPoint.X() = Frm().Left() + nOfstY;
    rPoint.Y() = Frm().Top() + nOfstX;
}

/**
 * Calculates the a limit value when switching from
 * vertical to horizontal layout.
 */
long SwTextFrm::SwitchVerticalToHorizontal( long nLimit ) const
{
    Point aTmp( nLimit, 0 );
    SwitchVerticalToHorizontal( aTmp );
    return aTmp.Y();
}

SwFrmSwapper::SwFrmSwapper( const SwTextFrm* pTextFrm, bool bSwapIfNotSwapped )
    : pFrm( pTextFrm ), bUndo( false )
{
    if ( pFrm->IsVertical() &&
        ( (   bSwapIfNotSwapped && ! pFrm->IsSwapped() ) ||
          ( ! bSwapIfNotSwapped && pFrm->IsSwapped() ) ) )
    {
        bUndo = true;
        const_cast<SwTextFrm*>(pFrm)->SwapWidthAndHeight();
    }
}

SwFrmSwapper::~SwFrmSwapper()
{
    if ( bUndo )
        const_cast<SwTextFrm*>(pFrm)->SwapWidthAndHeight();
}

void SwTextFrm::SwitchLTRtoRTL( SwRect& rRect ) const
{
    SWAP_IF_NOT_SWAPPED swap(const_cast<SwTextFrm *>(this));

    long nWidth = rRect.Width();
    rRect.Left( 2 * ( Frm().Left() + Prt().Left() ) +
                Prt().Width() - rRect.Right() - 1 );

    rRect.Width( nWidth );
}

void SwTextFrm::SwitchLTRtoRTL( Point& rPoint ) const
{
    SWAP_IF_NOT_SWAPPED swap(const_cast<SwTextFrm *>(this));

    rPoint.X() = 2 * ( Frm().Left() + Prt().Left() ) + Prt().Width() - rPoint.X() - 1;
}

SwLayoutModeModifier::SwLayoutModeModifier( const OutputDevice& rOutp ) :
        rOut( rOutp ), nOldLayoutMode( rOutp.GetLayoutMode() )
{
}

SwLayoutModeModifier::~SwLayoutModeModifier()
{
    const_cast<OutputDevice&>(rOut).SetLayoutMode( nOldLayoutMode );
}

void SwLayoutModeModifier::Modify( bool bChgToRTL )
{
    const_cast<OutputDevice&>(rOut).SetLayoutMode( bChgToRTL ?
                                         TEXT_LAYOUT_BIDI_STRONG | TEXT_LAYOUT_BIDI_RTL :
                                         TEXT_LAYOUT_BIDI_STRONG );
}

void SwLayoutModeModifier::SetAuto()
{
    const ComplexTextLayoutMode nNewLayoutMode = nOldLayoutMode & ~TEXT_LAYOUT_BIDI_STRONG;
    const_cast<OutputDevice&>(rOut).SetLayoutMode( nNewLayoutMode );
}

SwDigitModeModifier::SwDigitModeModifier( const OutputDevice& rOutp, LanguageType eCurLang ) :
        rOut( rOutp ), nOldLanguageType( rOutp.GetDigitLanguage() )
{
    LanguageType eLang = eCurLang;
    const SvtCTLOptions::TextNumerals nTextNumerals = SW_MOD()->GetCTLOptions().GetCTLTextNumerals();

    if ( SvtCTLOptions::NUMERALS_HINDI == nTextNumerals )
        eLang = LANGUAGE_ARABIC_SAUDI_ARABIA;
    else if ( SvtCTLOptions::NUMERALS_ARABIC == nTextNumerals )
        eLang = LANGUAGE_ENGLISH;
    else if ( SvtCTLOptions::NUMERALS_SYSTEM == nTextNumerals )
        eLang = ::GetAppLanguage();

    const_cast<OutputDevice&>(rOut).SetDigitLanguage( eLang );
}

SwDigitModeModifier::~SwDigitModeModifier()
{
    const_cast<OutputDevice&>(rOut).SetDigitLanguage( nOldLanguageType );
}

void SwTextFrm::Init()
{
    OSL_ENSURE( !IsLocked(), "+SwTextFrm::Init: this is locked." );
    if( !IsLocked() )
    {
        ClearPara();
        ResetBlinkPor();
        // set flags directly to save a ResetPreps call,
        // and thereby an unnecessary GetPara call
        // don't set bOrphan, bLocked or bWait to false!
        // bOrphan = bFlag7 = bFlag8 = false;
    }
}

SwTextFrm::SwTextFrm(SwTextNode * const pNode, SwFrm* pSib )
    : SwContentFrm( pNode, pSib )
    , nAllLines( 0 )
    , nThisLines( 0 )
    , mnFlyAnchorOfst( 0 )
    , mnFlyAnchorOfstNoWrap( 0 )
    , mnFootnoteLine( 0 )
    , mnHeightOfLastLine( 0 ) // OD 2004-03-17 #i11860#
    , mnAdditionalFirstLineOffset( 0 )
    , nOfst( 0 )
    , nCacheIdx( USHRT_MAX )
    , bLocked( false )
    , bWidow( false )
    , bJustWidow( false )
    , bEmpty( false )
    , bInFootnoteConnect( false )
    , bFootnote( false )
    , bRepaint( false )
    , bBlinkPor( false )
    , bFieldFollow( false )
    , bHasAnimation( false )
    , bIsSwapped( false )
    , mbFollowFormatAllowed( true ) // OD 14.03.2003 #i11760#
{
    mnFrmType = FRM_TXT;
}

void SwTextFrm::DestroyImpl()
{
    // Remove associated SwParaPortion from pTextCache
    ClearPara();

    const SwContentNode* pCNd;
    if( 0 != ( pCNd = PTR_CAST( SwContentNode, GetRegisteredIn() )) &&
        !pCNd->GetDoc()->IsInDtor() && HasFootnote() )
    {
        SwTextNode *pTextNd = static_cast<SwTextFrm*>(this)->GetTextNode();
        const SwFootnoteIdxs &rFootnoteIdxs = pCNd->GetDoc()->GetFootnoteIdxs();
        size_t nPos = 0;
        sal_uLong nIndex = pCNd->GetIndex();
        rFootnoteIdxs.SeekEntry( *pTextNd, &nPos );
        if( nPos < rFootnoteIdxs.size() )
        {
            while( nPos && pTextNd == &(rFootnoteIdxs[ nPos ]->GetTextNode()) )
                --nPos;
            if( nPos || pTextNd != &(rFootnoteIdxs[ nPos ]->GetTextNode()) )
                ++nPos;
        }
        while( nPos < rFootnoteIdxs.size() )
        {
            SwTextFootnote* pTextFootnote = rFootnoteIdxs[ nPos ];
            if( pTextFootnote->GetTextNode().GetIndex() > nIndex )
                break;
            pTextFootnote->DelFrms( this );
            ++nPos;
        }
    }

    SwContentFrm::DestroyImpl();
}

SwTextFrm::~SwTextFrm()
{
}

const OUString& SwTextFrm::GetText() const
{
    return GetTextNode()->GetText();
}

void SwTextFrm::ResetPreps()
{
    if ( GetCacheIdx() != USHRT_MAX )
    {
        SwParaPortion *pPara;
        if( 0 != (pPara = GetPara()) )
            pPara->ResetPreps();
    }
}

bool SwTextFrm::IsHiddenNow() const
{
    SwFrmSwapper aSwapper( this, true );

    if( !Frm().Width() && IsValid() && GetUpper()->IsValid() ) // invalid when stack overflows (StackHack)!
    {
//        OSL_FAIL( "SwTextFrm::IsHiddenNow: thin frame" );
        return true;
    }

    const bool bHiddenCharsHidePara = GetTextNode()->HasHiddenCharAttribute( true );
    const bool bHiddenParaField = GetTextNode()->HasHiddenParaField();
    const SwViewShell* pVsh = getRootFrm()->GetCurrShell();

    if ( pVsh && ( bHiddenCharsHidePara || bHiddenParaField ) )
    {
        if (
             ( bHiddenParaField &&
               ( !pVsh->GetViewOptions()->IsShowHiddenPara() &&
                 !pVsh->GetViewOptions()->IsFieldName() ) ) ||
             ( bHiddenCharsHidePara &&
               !pVsh->GetViewOptions()->IsShowHiddenChar() ) )
        {
            return true;
        }
    }

    return false;
}

/// Removes Textfrm's attachments, when it's hidden
void SwTextFrm::HideHidden()
{
    OSL_ENSURE( !GetFollow() && IsHiddenNow(),
            "HideHidden on visible frame of hidden frame has follow" );

    const sal_Int32 nEnd = COMPLETE_STRING;
    HideFootnotes( GetOfst(), nEnd );
    // OD 2004-01-15 #110582#
    HideAndShowObjects();

    // format information is obsolete
    ClearPara();
}

void SwTextFrm::HideFootnotes( sal_Int32 nStart, sal_Int32 nEnd )
{
    const SwpHints *pHints = GetTextNode()->GetpSwpHints();
    if( pHints )
    {
        const size_t nSize = pHints->Count();
        SwPageFrm *pPage = 0;
        for ( size_t i = 0; i < nSize; ++i )
        {
            const SwTextAttr *pHt = (*pHints)[i];
            if ( pHt->Which() == RES_TXTATR_FTN )
            {
                const sal_Int32 nIdx = pHt->GetStart();
                if ( nEnd < nIdx )
                    break;
                if( nStart <= nIdx )
                {
                    if( !pPage )
                        pPage = FindPageFrm();
                    pPage->RemoveFootnote( this, static_cast<const SwTextFootnote*>(pHt) );
                }
            }
        }
    }
}

/**
 * #120729# - hotfix
 * as-character anchored graphics, which are used for a graphic bullet list.
 * As long as these graphic bullet list aren't imported, do not hide a
 * at-character anchored object, if
 * (a) the document is an imported WW8 document -
 *     checked by checking certain compatibility options -
 * (b) the paragraph is the last content in the document and
 * (c) the anchor character is an as-character anchored graphic.
 */
bool sw_HideObj( const SwTextFrm& _rFrm,
                  const RndStdIds _eAnchorType,
                  const sal_Int32 _nObjAnchorPos,
                  SwAnchoredObject* _pAnchoredObj )
{
    bool bRet( true );

    if (_eAnchorType == FLY_AT_CHAR)
    {
        const IDocumentSettingAccess* pIDSA = _rFrm.GetTextNode()->getIDocumentSettingAccess();
        if ( !pIDSA->get(DocumentSettingId::USE_FORMER_TEXT_WRAPPING) &&
             !pIDSA->get(DocumentSettingId::OLD_LINE_SPACING) &&
             !pIDSA->get(DocumentSettingId::USE_FORMER_OBJECT_POS) &&
              pIDSA->get(DocumentSettingId::CONSIDER_WRAP_ON_OBJECT_POSITION) &&
             _rFrm.IsInDocBody() && !_rFrm.FindNextCnt() )
        {
            const OUString &rStr = _rFrm.GetTextNode()->GetText();
            const sal_Unicode cAnchorChar = _nObjAnchorPos < rStr.getLength() ? rStr[_nObjAnchorPos] : 0;
            if (cAnchorChar == CH_TXTATR_BREAKWORD)
            {
                const SwTextAttr* const pHint(
                    _rFrm.GetTextNode()->GetTextAttrForCharAt(_nObjAnchorPos,
                        RES_TXTATR_FLYCNT) );
                if ( pHint )
                {
                    const SwFrameFormat* pFrameFormat =
                        static_cast<const SwTextFlyCnt*>(pHint)->GetFlyCnt().GetFrameFormat();
                    if ( pFrameFormat->Which() == RES_FLYFRMFMT )
                    {
                        SwNodeIndex nContentIndex = *(pFrameFormat->GetContent().GetContentIdx());
                        ++nContentIndex;
                        if ( nContentIndex.GetNode().IsNoTextNode() )
                        {
                            bRet = false;
                            // set needed data structure values for object positioning
                            SWRECTFN( (&_rFrm) );
                            SwRect aLastCharRect( _rFrm.Frm() );
                            (aLastCharRect.*fnRect->fnSetWidth)( 1 );
                            _pAnchoredObj->maLastCharRect = aLastCharRect;
                            _pAnchoredObj->mnLastTopOfLine = (aLastCharRect.*fnRect->fnGetTop)();
                        }
                    }
                }
            }
        }
    }

    return bRet;
}

/**
 * Hide/show objects
 * OD 2004-01-15 #110582#
 *
 * Method hides respectively shows objects, which are anchored at paragraph,
 * at/as a character of the paragraph, corresponding to the paragraph and
 * paragraph portion visibility.
 *
 * - is called from HideHidden() - should hide objects in hidden paragraphs and
 * - from _Format() - should hide/show objects in partly visible paragraphs
 */
void SwTextFrm::HideAndShowObjects()
{
    if ( GetDrawObjs() )
    {
        if ( IsHiddenNow() )
        {
            // complete paragraph is hidden. Thus, hide all objects
            for ( size_t i = 0; i < GetDrawObjs()->size(); ++i )
            {
                SdrObject* pObj = (*GetDrawObjs())[i]->DrawObj();
                SwContact* pContact = static_cast<SwContact*>(pObj->GetUserCall());
                // #120729# - hotfix
                // under certain conditions
                const RndStdIds eAnchorType( pContact->GetAnchorId() );
                const sal_Int32 nObjAnchorPos = pContact->GetContentAnchorIndex().GetIndex();
                if ((eAnchorType != FLY_AT_CHAR) ||
                    sw_HideObj( *this, eAnchorType, nObjAnchorPos,
                                 (*GetDrawObjs())[i] ))
                {
                    pContact->MoveObjToInvisibleLayer( pObj );
                }
            }
        }
        else
        {
            // paragraph is visible, but can contain hidden text portion.
            // first we check if objects are allowed to be hidden:
            const SwTextNode& rNode = *GetTextNode();
            const SwViewShell* pVsh = getRootFrm()->GetCurrShell();
            const bool bShouldBeHidden = !pVsh || !pVsh->GetWin() ||
                                         !pVsh->GetViewOptions()->IsShowHiddenChar();

            // Thus, show all objects, which are anchored at paragraph and
            // hide/show objects, which are anchored at/as character, according
            // to the visibility of the anchor character.
            for ( size_t i = 0; i < GetDrawObjs()->size(); ++i )
            {
                SdrObject* pObj = (*GetDrawObjs())[i]->DrawObj();
                SwContact* pContact = static_cast<SwContact*>(pObj->GetUserCall());
                // #120729# - determine anchor type only once
                const RndStdIds eAnchorType( pContact->GetAnchorId() );

                if (eAnchorType == FLY_AT_PARA)
                {
                    pContact->MoveObjToVisibleLayer( pObj );
                }
                else if ((eAnchorType == FLY_AT_CHAR) ||
                         (eAnchorType == FLY_AS_CHAR))
                {
                    sal_Int32 nHiddenStart;
                    sal_Int32 nHiddenEnd;
                    const sal_Int32 nObjAnchorPos = pContact->GetContentAnchorIndex().GetIndex();
                    SwScriptInfo::GetBoundsOfHiddenRange( rNode, nObjAnchorPos, nHiddenStart, nHiddenEnd, 0 );
                    // #120729# - hotfix
                    // under certain conditions
                    if ( nHiddenStart != COMPLETE_STRING && bShouldBeHidden &&
                         sw_HideObj( *this, eAnchorType, nObjAnchorPos, (*GetDrawObjs())[i] ) )
                        pContact->MoveObjToInvisibleLayer( pObj );
                    else
                        pContact->MoveObjToVisibleLayer( pObj );
                }
                else
                {
                    OSL_FAIL( "<SwTextFrm::HideAndShowObjects()> - object not anchored at/inside paragraph!?" );
                }
            }
        }
    }

    if (IsFollow())
    {
        SwTextFrm *pMaster = FindMaster();
        OSL_ENSURE(pMaster, "SwTextFrm without master");
        if (pMaster)
            pMaster->HideAndShowObjects();
    }
}

/**
 * Returns the first possible break point in the current line.
 * This method is used in SwTextFrm::Format() to decide whether the previous
 * line has to be formatted as well.
 * nFound is <= nEndLine.
 */
sal_Int32 SwTextFrm::FindBrk( const OUString &rText,
                              const sal_Int32 nStart,
                              const sal_Int32 nEnd )
{
    sal_Int32 nFound = nStart;
    const sal_Int32 nEndLine = std::min( nEnd, rText.getLength() - 1 );

    // skip all leading blanks (see bug #2235).
    while( nFound <= nEndLine && ' ' == rText[nFound] )
    {
         nFound++;
    }

    // A tricky situation with the TextAttr-Dummy-character (in this case "$"):
    // "Dr.$Meyer" at the beginning of the second line. Typing a blank after that
    // doesn't result in the word moving into first line, even though that would work.
    // For this reason we don't skip the dummy char.
    while( nFound <= nEndLine && ' ' != rText[nFound] )
    {
        nFound++;
    }

    return nFound;
}

bool SwTextFrm::IsIdxInside( const sal_Int32 nPos, const sal_Int32 nLen ) const
{
    if( nLen != COMPLETE_STRING && GetOfst() > nPos + nLen ) // the range preceded us
        return false;

    if( !GetFollow() ) // the range doesn't precede us,
        return true; // nobody follows us.

    const sal_Int32 nMax = GetFollow()->GetOfst();

    // either the range overlap or our text has been deleted
    if( nMax > nPos || nMax > GetText().getLength() )
        return true;

    // changes made in the first line of a follow can modify the master
    const SwParaPortion* pPara = GetFollow()->GetPara();
    return pPara && ( nPos <= nMax + pPara->GetLen() );
}

inline void SwTextFrm::InvalidateRange(const SwCharRange &aRange, const long nD)
{
    if ( IsIdxInside( aRange.Start(), aRange.Len() ) )
        _InvalidateRange( aRange, nD );
}

void SwTextFrm::_InvalidateRange( const SwCharRange &aRange, const long nD)
{
    if ( !HasPara() )
    {   InvalidateSize();
        return;
    }

    SetWidow( false );
    SwParaPortion *pPara = GetPara();

    bool bInv = false;
    if( 0 != nD )
    {
        // In nDelta the differences between old and new
        // linelengths are being added, that's why it's negative
        // if chars have been added and positive, if chars have
        // deleted
        pPara->GetDelta() += nD;
        bInv = true;
    }
    SwCharRange &rReformat = pPara->GetReformat();
    if(aRange != rReformat) {
        if( COMPLETE_STRING == rReformat.Len() )
            rReformat = aRange;
        else
            rReformat += aRange;
        bInv = true;
    }
    if(bInv)
    {
        InvalidateSize();
    }
}

void SwTextFrm::CalcLineSpace()
{
    OSL_ENSURE( ! IsVertical() || ! IsSwapped(),
            "SwTextFrm::CalcLineSpace with swapped frame!" );

    if( IsLocked() || !HasPara() )
        return;

    SwParaPortion *pPara;
    if( GetDrawObjs() ||
        GetTextNode()->GetSwAttrSet().GetLRSpace().IsAutoFirst() ||
        ( pPara = GetPara() )->IsFixLineHeight() )
    {
        Init();
        return;
    }

    Size aNewSize( Prt().SSize() );

    SwTextFormatInfo aInf( getRootFrm()->GetCurrShell()->GetOut(), this );
    SwTextFormatter aLine( this, &aInf );
    if( aLine.GetDropLines() )
    {
        Init();
        return;
    }

    aLine.Top();
    aLine.RecalcRealHeight();

    aNewSize.Height() = (aLine.Y() - Frm().Top()) + aLine.GetLineHeight();

    SwTwips nDelta = aNewSize.Height() - Prt().Height();
    // Underflow with free-flying frames
    if( aInf.GetTextFly().IsOn() )
    {
        SwRect aTmpFrm( Frm() );
        if( nDelta < 0 )
            aTmpFrm.Height( Prt().Height() );
        else
            aTmpFrm.Height( aNewSize.Height() );
        if( aInf.GetTextFly().Relax( aTmpFrm ) )
        {
            Init();
            return;
        }
    }

    if( nDelta )
    {
        SwTextFrmBreak aBreak( this );
        if( GetFollow() || aBreak.IsBreakNow( aLine ) )
        {
            // if there is a Follow() or if we need to break here, reformat
            Init();
        }
        else
        {
            // everything is business as usual...
            pPara->SetPrepAdjust();
            pPara->SetPrep();
        }
    }
}

static void lcl_SetWrong( SwTextFrm& rFrm, sal_Int32 nPos, sal_Int32 nCnt, bool bMove )
{
    if ( !rFrm.IsFollow() )
    {
        SwTextNode* pTextNode = rFrm.GetTextNode();
        IGrammarContact* pGrammarContact = getGrammarContact( *pTextNode );
        SwGrammarMarkUp* pWrongGrammar = pGrammarContact ?
            pGrammarContact->getGrammarCheck( *pTextNode, false ) :
            pTextNode->GetGrammarCheck();
        bool bGrammarProxy = pWrongGrammar != pTextNode->GetGrammarCheck();
        if( bMove )
        {
            if( pTextNode->GetWrong() )
                pTextNode->GetWrong()->Move( nPos, nCnt );
            if( pWrongGrammar )
                pWrongGrammar->MoveGrammar( nPos, nCnt );
            if( bGrammarProxy && pTextNode->GetGrammarCheck() )
                pTextNode->GetGrammarCheck()->MoveGrammar( nPos, nCnt );
            if( pTextNode->GetSmartTags() )
                pTextNode->GetSmartTags()->Move( nPos, nCnt );
        }
        else
        {
            if( pTextNode->GetWrong() )
                pTextNode->GetWrong()->Invalidate( nPos, nCnt );
            if( pWrongGrammar )
                pWrongGrammar->Invalidate( nPos, nCnt );
            if( pTextNode->GetSmartTags() )
                pTextNode->GetSmartTags()->Invalidate( nPos, nCnt );
        }
        const sal_Int32 nEnd = nPos + (nCnt > 0 ? nCnt : 1 );
        if ( !pTextNode->GetWrong() && !pTextNode->IsWrongDirty() )
        {
            pTextNode->SetWrong( new SwWrongList( WRONGLIST_SPELL ) );
            pTextNode->GetWrong()->SetInvalid( nPos, nEnd );
        }
        if ( !pTextNode->GetSmartTags() && !pTextNode->IsSmartTagDirty() )
        {
            pTextNode->SetSmartTags( new SwWrongList( WRONGLIST_SMARTTAG ) );
            pTextNode->GetSmartTags()->SetInvalid( nPos, nEnd );
        }
        pTextNode->SetWrongDirty( true );
        pTextNode->SetGrammarCheckDirty( true );
        pTextNode->SetWordCountDirty( true );
        pTextNode->SetAutoCompleteWordDirty( true );
        pTextNode->SetSmartTagDirty( true );
    }

    SwRootFrm *pRootFrm = rFrm.getRootFrm();
    if (pRootFrm)
    {
        pRootFrm->SetNeedGrammarCheck( true );
    }

    SwPageFrm *pPage = rFrm.FindPageFrm();
    if( pPage )
    {
        pPage->InvalidateSpelling();
        pPage->InvalidateAutoCompleteWords();
        pPage->InvalidateWordCount();
        pPage->InvalidateSmartTags();
    }
}

static void lcl_SetScriptInval( SwTextFrm& rFrm, sal_Int32 nPos )
{
    if( rFrm.GetPara() )
        rFrm.GetPara()->GetScriptInfo().SetInvalidityA( nPos );
}

static void lcl_ModifyOfst( SwTextFrm* pFrm, sal_Int32 nPos, sal_Int32 nLen )
{
    while( pFrm && pFrm->GetOfst() <= nPos )
        pFrm = pFrm->GetFollow();
    while( pFrm )
    {
        if (nLen == COMPLETE_STRING)
            pFrm->ManipOfst( pFrm->GetTextNode()->GetText().getLength() );
        else
            pFrm->ManipOfst( pFrm->GetOfst() + nLen );
        pFrm = pFrm->GetFollow();
    }
}

/**
 * Related: fdo#56031 filter out attribute changes that don't matter for
 * humans/a11y to stop flooding the destination mortal with useless noise
 */
static bool isA11yRelevantAttribute(sal_uInt16 nWhich)
{
    return nWhich != RES_CHRATR_RSID;
}

static bool hasA11yRelevantAttribute( const std::vector<sal_uInt16>& nWhich )
{
    for( std::vector<sal_uInt16>::const_iterator nItr = nWhich.begin();
            nItr < nWhich.end(); ++nItr )
        if ( isA11yRelevantAttribute( *nItr ) )
            return true;

    return false;
}

void SwTextFrm::Modify( const SfxPoolItem* pOld, const SfxPoolItem *pNew )
{
    const sal_uInt16 nWhich = pOld ? pOld->Which() : pNew ? pNew->Which() : 0;

    // modifications concerning frame attributes are processed by the base class
    if( IsInRange( aFrameFormatSetRange, nWhich ) || RES_FMT_CHG == nWhich )
    {
        SwContentFrm::Modify( pOld, pNew );
        if( nWhich == RES_FMT_CHG && getRootFrm()->GetCurrShell() )
        {
            // collection has changed
            Prepare( PREP_CLEAR );
            _InvalidatePrt();
            lcl_SetWrong( *this, 0, COMPLETE_STRING, false );
            SetDerivedR2L( false );
            CheckDirChange();
            // OD 09.12.2002 #105576# - Force complete paint due to existing
            // indents.
            SetCompletePaint();
            InvalidateLineNum();
        }
        return;
    }

    // while locked ignore all modifications
    if( IsLocked() )
        return;

    // save stack
    // warning: one has to ensure that all variables are set
    sal_Int32 nPos;
    sal_Int32 nLen;
    bool bSetFieldsDirty = false;
    bool bRecalcFootnoteFlag = false;

    switch( nWhich )
    {
        case RES_LINENUMBER:
        {
            InvalidateLineNum();
        }
        break;
        case RES_INS_TXT:
        {
            nPos = static_cast<const SwInsText*>(pNew)->nPos;
            nLen = static_cast<const SwInsText*>(pNew)->nLen;
            if( IsIdxInside( nPos, nLen ) )
            {
                if( !nLen )
                {
                    // Refresh NumPortions even when line is empty!
                    if( nPos )
                        InvalidateSize();
                    else
                        Prepare( PREP_CLEAR );
                }
                else
                    _InvalidateRange( SwCharRange( nPos, nLen ), nLen );
            }
            lcl_SetWrong( *this, nPos, nLen, true );
            lcl_SetScriptInval( *this, nPos );
            bSetFieldsDirty = true;
            if( HasFollow() )
                lcl_ModifyOfst( this, nPos, nLen );
        }
        break;
        case RES_DEL_CHR:
        {
            nPos = static_cast<const SwDelChr*>(pNew)->nPos;
            InvalidateRange( SwCharRange( nPos, 1 ), -1 );
            lcl_SetWrong( *this, nPos, -1, true );
            lcl_SetScriptInval( *this, nPos );
            bSetFieldsDirty = bRecalcFootnoteFlag = true;
            if( HasFollow() )
                lcl_ModifyOfst( this, nPos, COMPLETE_STRING );
        }
        break;
        case RES_DEL_TXT:
        {
            nPos = static_cast<const SwDelText*>(pNew)->nStart;
            nLen = static_cast<const SwDelText*>(pNew)->nLen;
            const sal_Int32 m = -nLen;
            if( IsIdxInside( nPos, nLen ) )
            {
                if( !nLen )
                    InvalidateSize();
                else
                    InvalidateRange( SwCharRange( nPos, 1 ), m );
            }
            lcl_SetWrong( *this, nPos, m, true );
            lcl_SetScriptInval( *this, nPos );
            bSetFieldsDirty = bRecalcFootnoteFlag = true;
            if( HasFollow() )
                lcl_ModifyOfst( this, nPos, nLen );
        }
        break;
        case RES_UPDATE_ATTR:
        {
            nPos = static_cast<const SwUpdateAttr*>(pNew)->getStart();
            nLen = static_cast<const SwUpdateAttr*>(pNew)->getEnd() - nPos;
            if( IsIdxInside( nPos, nLen ) )
            {
                // We need to reformat anyways, even if the invalidated
                // area is NULL.
                // E.g.: empty line, set 14 pt!
                // if( !nLen ) nLen = 1;

                // FootnoteNummbers need to be formatted
                if( !nLen )
                    nLen = 1;

                _InvalidateRange( SwCharRange( nPos, nLen) );
                const sal_uInt16 nTmp = static_cast<const SwUpdateAttr*>(pNew)->getWhichAttr();

                if( ! nTmp || RES_TXTATR_CHARFMT == nTmp || RES_TXTATR_AUTOFMT == nTmp ||
                    RES_FMT_CHG == nTmp || RES_ATTRSET_CHG == nTmp )
                {
                    lcl_SetWrong( *this, nPos, nPos + nLen, false );
                    lcl_SetScriptInval( *this, nPos );
                }
            }

            if( isA11yRelevantAttribute( static_cast<const SwUpdateAttr*>(pNew)->getWhichAttr() ) &&
                    hasA11yRelevantAttribute( static_cast<const SwUpdateAttr*>(pNew)->getFormatAttr() ) )
            {
                // #i104008#
                SwViewShell* pViewSh = getRootFrm() ? getRootFrm()->GetCurrShell() : 0;
                if ( pViewSh  )
                {
                    pViewSh->InvalidateAccessibleParaAttrs( *this );
                }
            }
        }
        break;
        case RES_OBJECTDYING:
        break;

        case RES_PARATR_LINESPACING:
            {
                CalcLineSpace();
                InvalidateSize();
                _InvalidatePrt();
                if( IsInSct() && !GetPrev() )
                {
                    SwSectionFrm *pSect = FindSctFrm();
                    if( pSect->ContainsAny() == this )
                        pSect->InvalidatePrt();
                }

                // OD 09.01.2004 #i11859# - correction:
                //  (1) Also invalidate next frame on next page/column.
                //  (2) Skip empty sections and hidden paragraphs
                //  Thus, use method <InvalidateNextPrtArea()>
                InvalidateNextPrtArea();

                SetCompletePaint();
            }
            break;

        case RES_TXTATR_FIELD:
        case RES_TXTATR_ANNOTATION:
            {
                nPos = static_cast<const SwFormatField*>(pNew)->GetTextField()->GetStart();
                if( IsIdxInside( nPos, 1 ) )
                {
                    if( pNew == pOld )
                    {
                        // only repaint
                        // opt: invalidate window?
                        InvalidatePage();
                        SetCompletePaint();
                    }
                    else
                        _InvalidateRange( SwCharRange( nPos, 1 ) );
                }
                bSetFieldsDirty = true;
                // ST2
                if ( SwSmartTagMgr::Get().IsSmartTagsEnabled() )
                    lcl_SetWrong( *this, nPos, nPos + 1, false );
            }
            break;

        case RES_TXTATR_FTN :
        {
            nPos = static_cast<const SwFormatFootnote*>(pNew)->GetTextFootnote()->GetStart();
            if( IsInFootnote() || IsIdxInside( nPos, 1 ) )
                Prepare( PREP_FTN, static_cast<const SwFormatFootnote*>(pNew)->GetTextFootnote() );
            break;
        }

        case RES_ATTRSET_CHG:
        {
            InvalidateLineNum();

            const SwAttrSet& rNewSet = *static_cast<const SwAttrSetChg*>(pNew)->GetChgSet();
            const SfxPoolItem* pItem = 0;
            int nClear = 0;
            sal_uInt16 nCount = rNewSet.Count();

            if( SfxItemState::SET == rNewSet.GetItemState( RES_TXTATR_FTN, false, &pItem ))
            {
                nPos = static_cast<const SwFormatFootnote*>(pItem)->GetTextFootnote()->GetStart();
                if( IsIdxInside( nPos, 1 ) )
                    Prepare( PREP_FTN, pNew );
                nClear = 0x01;
                --nCount;
            }

            if( SfxItemState::SET == rNewSet.GetItemState( RES_TXTATR_FIELD, false, &pItem ))
            {
                nPos = static_cast<const SwFormatField*>(pItem)->GetTextField()->GetStart();
                if( IsIdxInside( nPos, 1 ) )
                {
                    const SfxPoolItem* pOldItem = pOld ?
                        &(static_cast<const SwAttrSetChg*>(pOld)->GetChgSet()->Get(RES_TXTATR_FIELD)) : NULL;
                    if( pItem == pOldItem )
                    {
                        InvalidatePage();
                        SetCompletePaint();
                    }
                    else
                        _InvalidateRange( SwCharRange( nPos, 1 ) );
                }
                nClear |= 0x02;
                --nCount;
            }
            bool bLineSpace = SfxItemState::SET == rNewSet.GetItemState(
                                            RES_PARATR_LINESPACING, false ),
                     bRegister  = SfxItemState::SET == rNewSet.GetItemState(
                                            RES_PARATR_REGISTER, false );
            if ( bLineSpace || bRegister )
            {
                Prepare( bRegister ? PREP_REGISTER : PREP_ADJUST_FRM );
                CalcLineSpace();
                InvalidateSize();
                _InvalidatePrt();

                // OD 09.01.2004 #i11859# - correction:
                //  (1) Also invalidate next frame on next page/column.
                //  (2) Skip empty sections and hidden paragraphs
                //  Thus, use method <InvalidateNextPrtArea()>
                InvalidateNextPrtArea();

                SetCompletePaint();
                nClear |= 0x04;
                if ( bLineSpace )
                {
                    --nCount;
                    if( IsInSct() && !GetPrev() )
                    {
                        SwSectionFrm *pSect = FindSctFrm();
                        if( pSect->ContainsAny() == this )
                            pSect->InvalidatePrt();
                    }
                }
                if ( bRegister )
                    --nCount;
            }
            if ( SfxItemState::SET == rNewSet.GetItemState( RES_PARATR_SPLIT,
                                                       false ))
            {
                if ( GetPrev() )
                    CheckKeep();
                Prepare( PREP_CLEAR );
                InvalidateSize();
                nClear |= 0x08;
                --nCount;
            }

            if( SfxItemState::SET == rNewSet.GetItemState( RES_BACKGROUND, false)
                && !IsFollow() && GetDrawObjs() )
            {
                SwSortedObjs *pObjs = GetDrawObjs();
                for ( size_t i = 0; GetDrawObjs() && i < pObjs->size(); ++i )
                {
                    SwAnchoredObject* pAnchoredObj = (*pObjs)[i];
                    if ( pAnchoredObj->ISA(SwFlyFrm) )
                    {
                        SwFlyFrm *pFly = static_cast<SwFlyFrm*>(pAnchoredObj);
                        if( !pFly->IsFlyInCntFrm() )
                        {
                            const SvxBrushItem &rBack =
                                pFly->GetAttrSet()->GetBackground();
                            // OD 20.08.2002 #99657# #GetTransChg#
                            //     following condition determines, if the fly frame
                            //     "inherites" the background color of text frame.
                            //     This is the case, if fly frame background
                            //     color is "no fill"/"auto fill" and if the fly frame
                            //     has no background graphic.
                            //     Thus, check complete fly frame background
                            //     color and *not* only its transparency value
                            if ( (rBack.GetColor() == COL_TRANSPARENT)  &&
                                rBack.GetGraphicPos() == GPOS_NONE )
                            {
                                pFly->SetCompletePaint();
                                pFly->InvalidatePage();
                            }
                        }
                    }
                }
            }

            if ( SfxItemState::SET ==
                 rNewSet.GetItemState( RES_TXTATR_CHARFMT, false ) )
            {
                lcl_SetWrong( *this, 0, COMPLETE_STRING, false );
                lcl_SetScriptInval( *this, 0 );
            }
            else if ( SfxItemState::SET ==
                      rNewSet.GetItemState( RES_CHRATR_LANGUAGE, false ) ||
                      SfxItemState::SET ==
                      rNewSet.GetItemState( RES_CHRATR_CJK_LANGUAGE, false ) ||
                      SfxItemState::SET ==
                      rNewSet.GetItemState( RES_CHRATR_CTL_LANGUAGE, false ) )
                lcl_SetWrong( *this, 0, COMPLETE_STRING, false );
            else if ( SfxItemState::SET ==
                      rNewSet.GetItemState( RES_CHRATR_FONT, false ) ||
                      SfxItemState::SET ==
                      rNewSet.GetItemState( RES_CHRATR_CJK_FONT, false ) ||
                      SfxItemState::SET ==
                      rNewSet.GetItemState( RES_CHRATR_CTL_FONT, false ) )
                lcl_SetScriptInval( *this, 0 );
            else if ( SfxItemState::SET ==
                      rNewSet.GetItemState( RES_FRAMEDIR, false ) )
            {
                SetDerivedR2L( false );
                CheckDirChange();
                // OD 09.12.2002 #105576# - Force complete paint due to existing
                // indents.
                SetCompletePaint();
            }

            if( nCount )
            {
                if( getRootFrm()->GetCurrShell() )
                {
                    Prepare( PREP_CLEAR );
                    _InvalidatePrt();
                }

                if( nClear )
                {
                    SwAttrSetChg aOldSet( *static_cast<const SwAttrSetChg*>(pOld) );
                    SwAttrSetChg aNewSet( *static_cast<const SwAttrSetChg*>(pNew) );

                    if( 0x01 & nClear )
                    {
                        aOldSet.ClearItem( RES_TXTATR_FTN );
                        aNewSet.ClearItem( RES_TXTATR_FTN );
                    }
                    if( 0x02 & nClear )
                    {
                        aOldSet.ClearItem( RES_TXTATR_FIELD );
                        aNewSet.ClearItem( RES_TXTATR_FIELD );
                    }
                    if ( 0x04 & nClear )
                    {
                        if ( bLineSpace )
                        {
                            aOldSet.ClearItem( RES_PARATR_LINESPACING );
                            aNewSet.ClearItem( RES_PARATR_LINESPACING );
                        }
                        if ( bRegister )
                        {
                            aOldSet.ClearItem( RES_PARATR_REGISTER );
                            aNewSet.ClearItem( RES_PARATR_REGISTER );
                        }
                    }
                    if ( 0x08 & nClear )
                    {
                        aOldSet.ClearItem( RES_PARATR_SPLIT );
                        aNewSet.ClearItem( RES_PARATR_SPLIT );
                    }
                    SwContentFrm::Modify( &aOldSet, &aNewSet );
                }
                else
                    SwContentFrm::Modify( pOld, pNew );
            }

            if (isA11yRelevantAttribute(nWhich))
            {
                // #i88069#
                SwViewShell* pViewSh = getRootFrm() ? getRootFrm()->GetCurrShell() : 0;
                if ( pViewSh  )
                {
                    pViewSh->InvalidateAccessibleParaAttrs( *this );
                }
            }
        }
        break;

        // Process SwDocPosUpdate
        case RES_DOCPOS_UPDATE:
        {
            if( pOld && pNew )
            {
                const SwDocPosUpdate *pDocPos = static_cast<const SwDocPosUpdate*>(pOld);
                if( pDocPos->nDocPos <= maFrm.Top() )
                {
                    const SwFormatField *pField = static_cast<const SwFormatField *>(pNew);
                    InvalidateRange(
                        SwCharRange( pField->GetTextField()->GetStart(), 1 ) );
                }
            }
            break;
        }
        case RES_PARATR_SPLIT:
            if ( GetPrev() )
                CheckKeep();
            Prepare( PREP_CLEAR );
            bSetFieldsDirty = true;
            break;
        case RES_FRAMEDIR :
            SetDerivedR2L( false );
            CheckDirChange();
            break;
        default:
        {
            Prepare( PREP_CLEAR );
            _InvalidatePrt();
            if ( !nWhich )
            {
                // is called by e. g. HiddenPara with 0
                SwFrm *pNxt;
                if ( 0 != (pNxt = FindNext()) )
                    pNxt->InvalidatePrt();
            }
        }
    } // switch

    if( bSetFieldsDirty )
        GetNode()->getIDocumentFieldsAccess().SetFieldsDirty( true, GetNode(), 1 );

    if ( bRecalcFootnoteFlag )
        CalcFootnoteFlag();
}

bool SwTextFrm::GetInfo( SfxPoolItem &rHint ) const
{
    if ( RES_VIRTPAGENUM_INFO == rHint.Which() && IsInDocBody() && ! IsFollow() )
    {
        SwVirtPageNumInfo &rInfo = static_cast<SwVirtPageNumInfo&>(rHint);
        const SwPageFrm *pPage = FindPageFrm();
        if ( pPage )
        {
            if ( pPage == rInfo.GetOrigPage() && !GetPrev() )
            {
                // this should be the one
                // (could only differ temporarily; is that disturbing?)
                rInfo.SetInfo( pPage, this );
                return false;
            }
            if ( pPage->GetPhyPageNum() < rInfo.GetOrigPage()->GetPhyPageNum() &&
                 (!rInfo.GetPage() || pPage->GetPhyPageNum() > rInfo.GetPage()->GetPhyPageNum()))
            {
                // this could be the one
                rInfo.SetInfo( pPage, this );
            }
        }
    }
    return true;
}

void SwTextFrm::PrepWidows( const sal_uInt16 nNeed, bool bNotify )
{
    OSL_ENSURE(GetFollow() && nNeed, "+SwTextFrm::Prepare: lost all friends");

    SwParaPortion *pPara = GetPara();
    if ( !pPara )
        return;
    pPara->SetPrepWidows();

    sal_uInt16 nHave = nNeed;

    // We yield a few lines and shrink in CalcPreps()
    SWAP_IF_NOT_SWAPPED swap( this );

    SwTextSizeInfo aInf( this );
    SwTextMargin aLine( this, &aInf );
    aLine.Bottom();
    sal_Int32 nTmpLen = aLine.GetCurr()->GetLen();
    while( nHave && aLine.PrevLine() )
    {
        if( nTmpLen )
            --nHave;
        nTmpLen = aLine.GetCurr()->GetLen();
    }

    // If it's certain that we can yield lines, the Master needs
    // to check the widow rule
    if( !nHave )
    {
        bool bSplit = true;
        if( !IsFollow() ) // only a master decides about orphans
        {
            const WidowsAndOrphans aWidOrp( this );
            bSplit = ( aLine.GetLineNr() >= aWidOrp.GetOrphansLines() &&
                       aLine.GetLineNr() >= aLine.GetDropLines() );
        }

        if( bSplit )
        {
            GetFollow()->SetOfst( aLine.GetEnd() );
            aLine.TruncLines( true );
            if( pPara->IsFollowField() )
                GetFollow()->SetFieldFollow( true );
        }
    }
    if ( bNotify )
    {
        _InvalidateSize();
        InvalidatePage();
    }
}

static bool lcl_ErgoVadis( SwTextFrm* pFrm, sal_Int32 &rPos, const PrepareHint ePrep )
{
    const SwFootnoteInfo &rFootnoteInfo = pFrm->GetNode()->GetDoc()->GetFootnoteInfo();
    if( ePrep == PREP_ERGOSUM )
    {
        if( rFootnoteInfo.aErgoSum.isEmpty() )
            return false;
        rPos = pFrm->GetOfst();
    }
    else
    {
        if( rFootnoteInfo.aQuoVadis.isEmpty() )
            return false;
        if( pFrm->HasFollow() )
            rPos = pFrm->GetFollow()->GetOfst();
        else
            rPos = pFrm->GetText().getLength();
        if( rPos )
            --rPos; // our last character
    }
    return true;
}

bool SwTextFrm::Prepare( const PrepareHint ePrep, const void* pVoid,
                        bool bNotify )
{
    bool bParaPossiblyInvalid = false;

    SwFrmSwapper aSwapper( this, false );

#if OSL_DEBUG_LEVEL > 1
    const SwTwips nDbgY = Frm().Top();
    (void)nDbgY;
#endif

    if ( IsEmpty() )
    {
        switch ( ePrep )
        {
            case PREP_BOSS_CHGD:
                SetInvalidVert( true ); // Test
            case PREP_WIDOWS_ORPHANS:
            case PREP_WIDOWS:
            case PREP_FTN_GONE :    return bParaPossiblyInvalid;

            case PREP_POS_CHGD :
            {
                // We also need an InvalidateSize for Areas (with and without columns),
                // so that we format and bUndersized is set (if needed)
                if( IsInFly() || IsInSct() )
                {
                    SwTwips nTmpBottom = GetUpper()->Frm().Top() +
                        GetUpper()->Prt().Bottom();
                    if( nTmpBottom < Frm().Bottom() )
                        break;
                }
                // Are there any free-flying frames on this page?
                SwTextFly aTextFly( this );
                if( aTextFly.IsOn() )
                {
                    // Does any free-flying frame overlap?
                    if ( aTextFly.Relax() || IsUndersized() )
                        break;
                }
                if( GetTextNode()->GetSwAttrSet().GetRegister().GetValue())
                    break;

                SwTextGridItem const*const pGrid(GetGridItem(FindPageFrm()));
                if ( pGrid && GetTextNode()->GetSwAttrSet().GetParaGrid().GetValue() )
                    break;

                // #i28701# - consider anchored objects
                if ( GetDrawObjs() )
                    break;

                return bParaPossiblyInvalid;
            }
            default:
                break;
        }
    }

    if( !HasPara() && PREP_MUST_FIT != ePrep )
    {
        SetInvalidVert( true ); // Test
        OSL_ENSURE( !IsLocked(), "SwTextFrm::Prepare: three of a perfect pair" );
        if ( bNotify )
            InvalidateSize();
        else
            _InvalidateSize();
        return bParaPossiblyInvalid;
    }

    // Get object from cache while locking
    SwTextLineAccess aAccess( this );
    SwParaPortion *pPara = aAccess.GetPara();

    switch( ePrep )
    {
        case PREP_MOVEFTN :     Frm().Height(0);
                                Prt().Height(0);
                                _InvalidatePrt();
                                _InvalidateSize();
                                /* no break here */
        case PREP_ADJUST_FRM :  pPara->SetPrepAdjust();
                                if( IsFootnoteNumFrm() != pPara->IsFootnoteNum() ||
                                    IsUndersized() )
                                {
                                    InvalidateRange( SwCharRange( 0, 1 ), 1);
                                    if( GetOfst() && !IsFollow() )
                                        _SetOfst( 0 );
                                }
                                break;
        case PREP_MUST_FIT :        pPara->SetPrepMustFit();
                                /* no break here */
        case PREP_WIDOWS_ORPHANS :  pPara->SetPrepAdjust();
                                    break;

        case PREP_WIDOWS :
            // MustFit is stronger than anything else
            if( pPara->IsPrepMustFit() )
                return bParaPossiblyInvalid;
            // see comment in WidowsAndOrphans::FindOrphans and CalcPreps()
            PrepWidows( *static_cast<const sal_uInt16 *>(pVoid), bNotify );
            break;

        case PREP_FTN :
        {
            SwTextFootnote const *pFootnote = static_cast<SwTextFootnote const *>(pVoid);
            if( IsInFootnote() )
            {
                // Am I the first TextFrm of a footnote?
                if( !GetPrev() )
                    // So we're a TextFrm of the footnote, which has
                    // to display the footnote number or the ErgoSum text
                    InvalidateRange( SwCharRange( 0, 1 ), 1);

                if( !GetNext() )
                {
                    // We're the last Footnote; we need to update the
                    // QuoVadis texts now
                    const SwFootnoteInfo &rFootnoteInfo = GetNode()->GetDoc()->GetFootnoteInfo();
                    if( !pPara->UpdateQuoVadis( rFootnoteInfo.aQuoVadis ) )
                    {
                        sal_Int32 nPos = pPara->GetParLen();
                        if( nPos )
                            --nPos;
                        InvalidateRange( SwCharRange( nPos, 1 ), 1);
                    }
                }
            }
            else
            {
                // We are the TextFrm _with_ the footnote
                const sal_Int32 nPos = pFootnote->GetStart();
                InvalidateRange( SwCharRange( nPos, 1 ), 1);
            }
            break;
        }
        case PREP_BOSS_CHGD :
        {
            // Test
            {
                SetInvalidVert( false );
                bool bOld = IsVertical();
                SetInvalidVert( true );
                if( bOld != IsVertical() )
                    InvalidateRange( SwCharRange( GetOfst(), COMPLETE_STRING ) );
            }

            if( HasFollow() )
            {
                sal_Int32 nNxtOfst = GetFollow()->GetOfst();
                if( nNxtOfst )
                    --nNxtOfst;
                InvalidateRange( SwCharRange( nNxtOfst, 1 ), 1);
            }
            if( IsInFootnote() )
            {
                sal_Int32 nPos;
                if( lcl_ErgoVadis( this, nPos, PREP_QUOVADIS ) )
                    InvalidateRange( SwCharRange( nPos, 1 ), 0 );
                if( lcl_ErgoVadis( this, nPos, PREP_ERGOSUM ) )
                    InvalidateRange( SwCharRange( nPos, 1 ), 0 );
            }
            // If we have a page number field, we must invalidate those spots
            SwpHints *pHints = GetTextNode()->GetpSwpHints();
            if( pHints )
            {
                const size_t nSize = pHints->Count();
                const sal_Int32 nEnd = GetFollow() ?
                                    GetFollow()->GetOfst() : COMPLETE_STRING;
                for ( size_t i = 0; i < nSize; ++i )
                {
                    const SwTextAttr *pHt = (*pHints)[i];
                    const sal_Int32 nStart = pHt->GetStart();
                    if( nStart >= GetOfst() )
                    {
                        if( nStart >= nEnd )
                            break;

                // If we're flowing back and own a Footnote, the Footnote also flows
                // with us. So that it doesn't obstruct us, we send ourselves
                // a ADJUST_FRM.
                // pVoid != 0 means MoveBwd()
                        const sal_uInt16 nWhich = pHt->Which();
                        if( RES_TXTATR_FIELD == nWhich ||
                            (HasFootnote() && pVoid && RES_TXTATR_FTN == nWhich))
                        InvalidateRange( SwCharRange( nStart, 1 ), 1 );
                    }
                }
            }
            // A new boss, a new chance for growing
            if( IsUndersized() )
            {
                _InvalidateSize();
                InvalidateRange( SwCharRange( GetOfst(), 1 ), 1);
            }
            break;
        }

        case PREP_POS_CHGD :
        {
            if ( GetValidPrtAreaFlag() )
            {
                SwTextGridItem const*const pGrid(GetGridItem(FindPageFrm()));
                if ( pGrid && GetTextNode()->GetSwAttrSet().GetParaGrid().GetValue() )
                    InvalidatePrt();
            }

            // If we don't overlap with anybody:
            // did any free-flying frame overlapped _before_ the position change?
            bool bFormat = pPara->HasFly();
            if( !bFormat )
            {
                if( IsInFly() )
                {
                    SwTwips nTmpBottom = GetUpper()->Frm().Top() +
                        GetUpper()->Prt().Bottom();
                    if( nTmpBottom < Frm().Bottom() )
                        bFormat = true;
                }
                if( !bFormat )
                {
                    if ( GetDrawObjs() )
                    {
                        const size_t nCnt = GetDrawObjs()->size();
                        for ( size_t i = 0; i < nCnt; ++i )
                        {
                            SwAnchoredObject* pAnchoredObj = (*GetDrawObjs())[i];
                            // #i28701# - consider all
                            // to-character anchored objects
                            if ( pAnchoredObj->GetFrameFormat().GetAnchor().GetAnchorId()
                                    == FLY_AT_CHAR )
                            {
                                bFormat = true;
                                break;
                            }
                        }
                    }
                    if( !bFormat )
                    {
                        // Are there any free-flying frames on this page?
                        SwTextFly aTextFly( this );
                        if( aTextFly.IsOn() )
                        {
                            // Does any free-flying frame overlap?
                            bFormat = aTextFly.Relax() || IsUndersized();
                        }
                    }
                }
            }

            if( bFormat )
            {
                if( !IsLocked() )
                {
                    if( pPara->GetRepaint().HasArea() )
                        SetCompletePaint();
                    Init();
                    pPara = 0;
                    _InvalidateSize();
                }
            }
            else
            {
                if( GetTextNode()->GetSwAttrSet().GetRegister().GetValue() )
                    bParaPossiblyInvalid = Prepare( PREP_REGISTER, 0, bNotify );
                // The Frames need to be readjusted, which caused by changes
                // in position
                else if( HasFootnote() )
                {
                    bParaPossiblyInvalid = Prepare( PREP_ADJUST_FRM, 0, bNotify );
                    _InvalidateSize();
                }
                else
                    return bParaPossiblyInvalid; // So that there's no SetPrep()

                if (bParaPossiblyInvalid)
                {
                    // It's possible that pPara was deleted above; retrieve it again
                    pPara = aAccess.GetPara();
                }

            }
            break;
        }
        case PREP_REGISTER:
            if( GetTextNode()->GetSwAttrSet().GetRegister().GetValue() )
            {
                pPara->SetPrepAdjust();
                CalcLineSpace();

                // It's possible that pPara was deleted above; retrieve it again
                bParaPossiblyInvalid = true;
                pPara = aAccess.GetPara();

                InvalidateSize();
                _InvalidatePrt();
                SwFrm* pNxt;
                if ( 0 != ( pNxt = GetIndNext() ) )
                {
                    pNxt->_InvalidatePrt();
                    if ( pNxt->IsLayoutFrm() )
                        pNxt->InvalidatePage();
                }
                SetCompletePaint();
            }
            break;
        case PREP_FTN_GONE :
            {
                // If a Follow is calling us, because a footnote is being deleted, our last
                // line has to be formatted, so that the first line of the Follow can flow up.
                // Which had flowed to the next page to be together with the footnote (this is
                // especially true for areas with columns)
                OSL_ENSURE( GetFollow(), "PREP_FTN_GONE darf nur vom Follow gerufen werden" );
                sal_Int32 nPos = GetFollow()->GetOfst();
                if( IsFollow() && GetOfst() == nPos )       // If we don't have a mass of text, we call our
                    FindMaster()->Prepare( PREP_FTN_GONE ); // Master's Prepare
                if( nPos )
                    --nPos; // The char preceding our Follow
                InvalidateRange( SwCharRange( nPos, 1 ), 0 );
                return bParaPossiblyInvalid;
            }
        case PREP_ERGOSUM:
        case PREP_QUOVADIS:
            {
                sal_Int32 nPos;
                if( lcl_ErgoVadis( this, nPos, ePrep ) )
                    InvalidateRange( SwCharRange( nPos, 1 ), 0 );
            }
            break;
        case PREP_FLY_ATTR_CHG:
        {
            if( pVoid )
            {
                sal_Int32 nWhere = CalcFlyPos( const_cast<SwFrameFormat *>(static_cast<SwFrameFormat const *>(pVoid)) );
                OSL_ENSURE( COMPLETE_STRING != nWhere, "Prepare: Why me?" );
                InvalidateRange( SwCharRange( nWhere, 1 ) );
                return bParaPossiblyInvalid;
            }
            // else: continue with default case block
        }
        case PREP_CLEAR:
        default:
        {
            if( IsLocked() )
            {
                if( PREP_FLY_ARRIVE == ePrep || PREP_FLY_LEAVE == ePrep )
                {
                    sal_Int32 nLen = ( GetFollow() ? GetFollow()->GetOfst() :
                                      COMPLETE_STRING ) - GetOfst();
                    InvalidateRange( SwCharRange( GetOfst(), nLen ), 0 );
                }
            }
            else
            {
                if( pPara->GetRepaint().HasArea() )
                    SetCompletePaint();
                Init();
                pPara = 0;
                if( GetOfst() && !IsFollow() )
                    _SetOfst( 0 );
                if ( bNotify )
                    InvalidateSize();
                else
                    _InvalidateSize();
            }
            return bParaPossiblyInvalid; // no SetPrep() happened
        }
    }
    if( pPara )
    {
        pPara->SetPrep();
    }

    return bParaPossiblyInvalid;
}

/**
 * Small Helper class:
 * Prepares a test format.
 * The frame is changed in size and position, its SwParaPortion is moved aside
 * and a new one is created.
 * To achieve this, run formatting with bTestFormat flag set.
 * In the destructor the TextFrm is reset to its original state.
 */
class SwTestFormat
{
    SwTextFrm *pFrm;
    SwParaPortion *pOldPara;
    SwRect aOldFrm, aOldPrt;
public:
    SwTestFormat( SwTextFrm* pTextFrm, const SwFrm* pPrv, SwTwips nMaxHeight );
    ~SwTestFormat();
};

SwTestFormat::SwTestFormat( SwTextFrm* pTextFrm, const SwFrm* pPre, SwTwips nMaxHeight )
    : pFrm( pTextFrm )
{
    aOldFrm = pFrm->Frm();
    aOldPrt = pFrm->Prt();

    SWRECTFN( pFrm )
    SwTwips nLower = (pFrm->*fnRect->fnGetBottomMargin)();

    pFrm->Frm() = pFrm->GetUpper()->Prt();
    pFrm->Frm() += pFrm->GetUpper()->Frm().Pos();

    (pFrm->Frm().*fnRect->fnSetHeight)( nMaxHeight );
    if( pFrm->GetPrev() )
        (pFrm->Frm().*fnRect->fnSetPosY)(
                (pFrm->GetPrev()->Frm().*fnRect->fnGetBottom)() -
                ( bVert ? nMaxHeight + 1 : 0 ) );

    SwBorderAttrAccess aAccess( SwFrm::GetCache(), pFrm );
    const SwBorderAttrs &rAttrs = *aAccess.Get();
    (pFrm->Prt().*fnRect->fnSetPosX)( rAttrs.CalcLeft( pFrm ) );

    if( pPre )
    {
        SwTwips nUpper = pFrm->CalcUpperSpace( &rAttrs, pPre );
        (pFrm->Prt().*fnRect->fnSetPosY)( nUpper );
    }
    (pFrm->Prt().*fnRect->fnSetHeight)(
        std::max( 0L , (pFrm->Frm().*fnRect->fnGetHeight)() -
                  (pFrm->Prt().*fnRect->fnGetTop)() - nLower ) );
    (pFrm->Prt().*fnRect->fnSetWidth)(
        (pFrm->Frm().*fnRect->fnGetWidth)() -
        ( rAttrs.CalcLeft( pFrm ) + rAttrs.CalcRight( pFrm ) ) );
    pOldPara = pFrm->HasPara() ? pFrm->GetPara() : NULL;
    pFrm->SetPara( new SwParaPortion(), false );

    OSL_ENSURE( ! pFrm->IsSwapped(), "A frame is swapped before _Format" );

    if ( pFrm->IsVertical() )
        pFrm->SwapWidthAndHeight();

    SwTextFormatInfo aInf( pFrm->getRootFrm()->GetCurrShell()->GetOut(), pFrm, false, true, true );
    SwTextFormatter  aLine( pFrm, &aInf );

    pFrm->_Format( aLine, aInf );

    if ( pFrm->IsVertical() )
        pFrm->SwapWidthAndHeight();

    OSL_ENSURE( ! pFrm->IsSwapped(), "A frame is swapped after _Format" );
}

SwTestFormat::~SwTestFormat()
{
    pFrm->Frm() = aOldFrm;
    pFrm->Prt() = aOldPrt;
    pFrm->SetPara( pOldPara );
}

bool SwTextFrm::TestFormat( const SwFrm* pPrv, SwTwips &rMaxHeight, bool &bSplit )
{
    PROTOCOL_ENTER( this, PROT_TESTFORMAT, 0, 0 )

    if( IsLocked() && GetUpper()->Prt().Width() <= 0 )
        return false;

    SwTestFormat aSave( this, pPrv, rMaxHeight );

    return SwTextFrm::WouldFit( rMaxHeight, bSplit, true );
}

/**
 * We should not and don't need to reformat.
 * We assume that we already formatted and that the formatting
 * data is still current.
 *
 * We also assume that the frame width of the Master and Follow
 * are the same. That's why we're not calling FindBreak() for
 * FindOrphans().
 * The required height is coming from nMaxHeight.
 *
 * @returns true if I can split
 */
bool SwTextFrm::WouldFit( SwTwips &rMaxHeight, bool &bSplit, bool bTst )
{
    OSL_ENSURE( ! IsVertical() || ! IsSwapped(),
            "SwTextFrm::WouldFit with swapped frame" );
    SWRECTFN( this );

    if( IsLocked() )
        return false;

    // it can happen that the IdleCollector removed the cached information
    if( !IsEmpty() )
        GetFormatted();

    // OD 2004-05-24 #i27801# - correction: 'short cut' for empty paragraph
    // can *not* be applied, if test format is in progress. The test format doesn't
    // adjust the frame and the printing area - see method <SwTextFrm::_Format(..)>,
    // which is called in <SwTextFrm::TestFormat(..)>
    if ( IsEmpty() && !bTst )
    {
        bSplit = false;
        SwTwips nHeight = bVert ? Prt().SSize().Width() : Prt().SSize().Height();
        if( rMaxHeight < nHeight )
            return false;
        else
        {
            rMaxHeight -= nHeight;
            return true;
        }
    }

    // GetPara can still be 0 in edge cases
    // We return true in order to be reformatted on the new Page
    OSL_ENSURE( HasPara() || IsHiddenNow(), "WouldFit: GetFormatted() and then !HasPara()" );
    if( !HasPara() || ( !(Frm().*fnRect->fnGetHeight)() && IsHiddenNow() ) )
        return true;

    // Because the Orphan flag only exists for a short moment, we also check
    // whether the Framesize is set to very huge by CalcPreps, in order to
    // force a MoveFwd
    if( IsWidow() || ( bVert ?
                       ( 0 == Frm().Left() ) :
                       ( LONG_MAX - 20000 < Frm().Bottom() ) ) )
    {
        SetWidow(false);
        if ( GetFollow() )
        {
            // If we've ended up here due to a Widow request by our Follow, we check
            // whether there's a Follow with a real height at all.
            // Else (e.g. for newly created SctFrms) we ignore the IsWidow() and
            // still check if we can find enough room
            if( ( ( ! bVert && LONG_MAX - 20000 >= Frm().Bottom() ) ||
                  (   bVert && 0 < Frm().Left() ) ) &&
                  ( GetFollow()->IsVertical() ?
                    !GetFollow()->Frm().Width() :
                    !GetFollow()->Frm().Height() ) )
            {
                SwTextFrm* pFoll = GetFollow()->GetFollow();
                while( pFoll &&
                        ( pFoll->IsVertical() ?
                         !pFoll->Frm().Width() :
                         !pFoll->Frm().Height() ) )
                    pFoll = pFoll->GetFollow();
                if( pFoll )
                    return false;
            }
            else
                return false;
        }
    }

    SWAP_IF_NOT_SWAPPED swap( this );

    SwTextSizeInfo aInf( this );
    SwTextMargin aLine( this, &aInf );

    WidowsAndOrphans aFrmBreak( this, rMaxHeight, bSplit );

    bool bRet = true;

    aLine.Bottom();
    // is breaking necessary?
    bSplit = !aFrmBreak.IsInside( aLine );
    if ( bSplit )
        bRet = !aFrmBreak.IsKeepAlways() && aFrmBreak.WouldFit( aLine, rMaxHeight, bTst );
    else
    {
        // we need the total height including the current line
        aLine.Top();
        do
        {
            rMaxHeight -= aLine.GetLineHeight();
        } while ( aLine.Next() );
    }

    return bRet;
}

sal_uInt16 SwTextFrm::GetParHeight() const
{
    OSL_ENSURE( ! IsVertical() || ! IsSwapped(),
            "SwTextFrm::GetParHeight with swapped frame" );

    if( !HasPara() )
    {   // For non-empty paragraphs this is a special case
        // For UnderSized we can simply just ask 1 Twip more
        sal_uInt16 nRet = (sal_uInt16)Prt().SSize().Height();
        if( IsUndersized() )
        {
            if( IsEmpty() || GetText().isEmpty() )
                nRet = (sal_uInt16)EmptyHeight();
            else
                ++nRet;
        }
        return nRet;
    }

    // TODO: Refactor and improve code
    const SwLineLayout* pLineLayout = GetPara();
    sal_uInt16 nHeight = pLineLayout ? pLineLayout->GetRealHeight() : 0;

    // Is this paragraph scrolled? Our height until now is at least
    // one line height too low then
    if( GetOfst() && !IsFollow() )
        nHeight *= 2;

    // OD 2004-03-04 #115793#
    while ( pLineLayout && pLineLayout->GetNext() )
    {
        pLineLayout = pLineLayout->GetNext();
        nHeight = nHeight + pLineLayout->GetRealHeight();
    }

    return nHeight;
}

/**
 * @returns this _always_ in the formatted state!
 */
SwTextFrm* SwTextFrm::GetFormatted( bool bForceQuickFormat )
{
    vcl::RenderContext* pRenderContext = getRootFrm()->GetCurrShell()->GetOut();
    SWAP_IF_SWAPPED swap( this );

    // The IdleCollector could've removed my cached information
    // Calc() calls our format
    // Not for empty paragraphs
    if( !HasPara() && !(IsValid() && IsEmpty()) )
    {
        // Calc() must be called, because frame position can be wrong
        const bool bFormat = GetValidSizeFlag();
        Calc(pRenderContext);

        // It could be that Calc() did not trigger Format(), because
        // we've been asked by the IdleCollector to throw away our
        // format information
        // Optimization with FormatQuick()
        if( bFormat && !FormatQuick( bForceQuickFormat ) )
            Format(getRootFrm()->GetCurrShell()->GetOut());
    }

    return this;
}

SwTwips SwTextFrm::CalcFitToContent()
{
    // #i31490#
    // If we are currently locked, we better return with a
    // fairly reasonable value:
    if ( IsLocked() )
        return Prt().Width();

    SwParaPortion* pOldPara = GetPara();
    SwParaPortion *pDummy = new SwParaPortion();
    SetPara( pDummy, false );
    const SwPageFrm* pPage = FindPageFrm();

    const Point   aOldFrmPos   = Frm().Pos();
    const SwTwips nOldFrmWidth = Frm().Width();
    const SwTwips nOldPrtWidth = Prt().Width();
    const SwTwips nPageWidth = GetUpper()->IsVertical() ?
                               pPage->Prt().Height() :
                               pPage->Prt().Width();

    Frm().Width( nPageWidth );
    Prt().Width( nPageWidth );

    // #i25422# objects anchored as character in RTL
    if ( IsRightToLeft() )
        Frm().Pos().X() += nOldFrmWidth - nPageWidth;

    // #i31490#
    TextFrmLockGuard aLock( this );

    SwTextFormatInfo aInf( getRootFrm()->GetCurrShell()->GetOut(), this, false, true, true );
    aInf.SetIgnoreFly( true );
    SwTextFormatter  aLine( this, &aInf );
    SwHookOut aHook( aInf );

    // #i54031# - assure mininum of MINLAY twips.
    const SwTwips nMax = std::max( (SwTwips)MINLAY,
                              aLine._CalcFitToContent() + 1 );

    Frm().Width( nOldFrmWidth );
    Prt().Width( nOldPrtWidth );

    // #i25422# objects anchored as character in RTL
    if ( IsRightToLeft() )
        Frm().Pos() = aOldFrmPos;

    SetPara( pOldPara );

    return nMax;
}

/**
 * Simulate format for a list item paragraph, whose list level attributes
 * are in LABEL_ALIGNMENT mode, in order to determine additional first
 * line offset for the real text formatting due to the value of label
 * adjustment attribute of the list level.
 */
void SwTextFrm::CalcAdditionalFirstLineOffset()
{
    if ( IsLocked() )
        return;

    // reset additional first line offset
    mnAdditionalFirstLineOffset = 0;

    const SwTextNode* pTextNode( GetTextNode() );
    if ( pTextNode && pTextNode->IsNumbered() && pTextNode->IsCountedInList() &&
         pTextNode->GetNumRule() )
    {
        int nListLevel = pTextNode->GetActualListLevel();

        if (nListLevel < 0)
            nListLevel = 0;

        if (nListLevel >= MAXLEVEL)
            nListLevel = MAXLEVEL - 1;

        const SwNumFormat& rNumFormat =
                pTextNode->GetNumRule()->Get( static_cast<sal_uInt16>(nListLevel) );
        if ( rNumFormat.GetPositionAndSpaceMode() == SvxNumberFormat::LABEL_ALIGNMENT )
        {
            // keep current paragraph portion and apply dummy paragraph portion
            SwParaPortion* pOldPara = GetPara();
            SwParaPortion *pDummy = new SwParaPortion();
            SetPara( pDummy, false );

            // lock paragraph
            TextFrmLockGuard aLock( this );

            // simulate text formatting
            SwTextFormatInfo aInf( getRootFrm()->GetCurrShell()->GetOut(), this, false, true, true );
            aInf.SetIgnoreFly( true );
            SwTextFormatter aLine( this, &aInf );
            SwHookOut aHook( aInf );
            aLine._CalcFitToContent();

            // determine additional first line offset
            const SwLinePortion* pFirstPortion = aLine.GetCurr()->GetFirstPortion();
            if ( pFirstPortion->InNumberGrp() && !pFirstPortion->IsFootnoteNumPortion() )
            {
                SwTwips nNumberPortionWidth( pFirstPortion->Width() );

                const SwLinePortion* pPortion = pFirstPortion->GetPortion();
                while ( pPortion &&
                        pPortion->InNumberGrp() && !pPortion->IsFootnoteNumPortion())
                {
                    nNumberPortionWidth += pPortion->Width();
                    pPortion = pPortion->GetPortion();
                }

                if ( ( IsRightToLeft() &&
                       rNumFormat.GetNumAdjust() == SVX_ADJUST_LEFT ) ||
                     ( !IsRightToLeft() &&
                       rNumFormat.GetNumAdjust() == SVX_ADJUST_RIGHT ) )
                {
                    mnAdditionalFirstLineOffset = -nNumberPortionWidth;
                }
                else if ( rNumFormat.GetNumAdjust() == SVX_ADJUST_CENTER )
                {
                    mnAdditionalFirstLineOffset = -(nNumberPortionWidth/2);
                }
            }

            // restore paragraph portion
            SetPara( pOldPara );
        }
    }
}

/**
 * Determine the height of the last line for the calculation of
 * the proportional line spacing
 *
 * Height of last line will be stored in new member
 * mnHeightOfLastLine and can be accessed via method
 * GetHeightOfLastLine()
 *
 * @param _bUseFont force the usage of the former algorithm to
 *                  determine the height of the last line, which
 *                  uses the font
 */
void SwTextFrm::_CalcHeightOfLastLine( const bool _bUseFont )
{
    // #i71281#
    // invalidate printing area, if height of last line changes
    const SwTwips mnOldHeightOfLastLine( mnHeightOfLastLine );

    // determine output device
    SwViewShell* pVsh = getRootFrm()->GetCurrShell();
    OSL_ENSURE( pVsh, "<SwTextFrm::_GetHeightOfLastLineForPropLineSpacing()> - no SwViewShell" );

    // #i78921# - make code robust, according to provided patch
    // There could be no <SwViewShell> instance in the case of loading a binary
    // StarOffice file format containing an embedded Writer document.
    if ( !pVsh )
    {
        return;
    }
    OutputDevice* pOut = pVsh->GetOut();
    const IDocumentSettingAccess* pIDSA = GetTextNode()->getIDocumentSettingAccess();
    if ( !pVsh->GetViewOptions()->getBrowseMode() ||
          pVsh->GetViewOptions()->IsPrtFormat() )
    {
        pOut = GetTextNode()->getIDocumentDeviceAccess().getReferenceDevice( true );
    }
    OSL_ENSURE( pOut, "<SwTextFrm::_GetHeightOfLastLineForPropLineSpacing()> - no OutputDevice" );
    // #i78921# - make code robust, according to provided patch
    if ( !pOut )
    {
        return;
    }

    // determine height of last line
    if ( _bUseFont || pIDSA->get(DocumentSettingId::OLD_LINE_SPACING ) )
    {
        // former determination of last line height for proprotional line
        // spacing - take height of font set at the paragraph
        SwFont aFont( GetAttrSet(), pIDSA );

        // we must ensure that the font is restored correctly on the OutputDevice
        // otherwise Last!=Owner could occur
        if ( pLastFont )
        {
            SwFntObj *pOldFont = pLastFont;
            pLastFont = NULL;
            aFont.SetFntChg( true );
            aFont.ChgPhysFnt( pVsh, *pOut );
            mnHeightOfLastLine = aFont.GetHeight( pVsh, *pOut );
            //coverity[var_deref_model] - pLastFont is set in SwSubFont::ChgFnt
            pLastFont->Unlock();
            pLastFont = pOldFont;
            pLastFont->SetDevFont( pVsh, *pOut );
        }
        else
        {
            vcl::Font aOldFont = pOut->GetFont();
            aFont.SetFntChg( true );
            aFont.ChgPhysFnt( pVsh, *pOut );
            mnHeightOfLastLine = aFont.GetHeight( pVsh, *pOut );
            //coverity[var_deref_model] - pLastFont is set in SwSubFont::ChgFnt
            pLastFont->Unlock();
            pLastFont = NULL;
            pOut->SetFont( aOldFont );
        }
    }
    else
    {
        // new determination of last line height - take actually height of last line
        // #i89000#
        // assure same results, if paragraph is undersized
        if ( IsUndersized() )
        {
            mnHeightOfLastLine = 0;
        }
        else
        {
            bool bCalcHeightOfLastLine = true;
            if ( ( !HasPara() && IsEmpty( ) ) || GetText().isEmpty() )
            {
                mnHeightOfLastLine = EmptyHeight();
                bCalcHeightOfLastLine = false;
            }

            if ( bCalcHeightOfLastLine )
            {
                OSL_ENSURE( HasPara(),
                        "<SwTextFrm::_CalcHeightOfLastLine()> - missing paragraph portions." );
                const SwLineLayout* pLineLayout = GetPara();
                while ( pLineLayout && pLineLayout->GetNext() )
                {
                    // iteration to last line
                    pLineLayout = pLineLayout->GetNext();
                }
                if ( pLineLayout )
                {
                    SwTwips nAscent, nDescent, nDummy1, nDummy2;
                    // #i47162# - suppress consideration of
                    // fly content portions and the line portion.
                    pLineLayout->MaxAscentDescent( nAscent, nDescent,
                                                   nDummy1, nDummy2,
                                                   0, true );
                    // #i71281#
                    // Suppress wrong invalidation of printing area, if method is
                    // called recursive.
                    // Thus, member <mnHeightOfLastLine> is only set directly, if
                    // no recursive call is needed.
                    const SwTwips nNewHeightOfLastLine = nAscent + nDescent;
                    // #i47162# - if last line only contains
                    // fly content portions, <mnHeightOfLastLine> is zero.
                    // In this case determine height of last line by the font
                    if ( nNewHeightOfLastLine == 0 )
                    {
                        _CalcHeightOfLastLine( true );
                    }
                    else
                    {
                        mnHeightOfLastLine = nNewHeightOfLastLine;
                    }
                }
            }
        }
    }
    // #i71281#
    // invalidate printing area, if height of last line changes
    if ( mnHeightOfLastLine != mnOldHeightOfLastLine )
    {
        InvalidatePrt();
    }
}

/**
 * Method returns the value of the inter line spacing for a text frame.
 * Such a value exists for proportional line spacings ("1,5 Lines",
 * "Double", "Proportional" and for leading line spacing ("Leading").
 *
 * @param _bNoPropLineSpacing (default = false) control whether the
 *                            value of a proportional line spacing is
 *                            returned or not
 */
long SwTextFrm::GetLineSpace( const bool _bNoPropLineSpace ) const
{
    long nRet = 0;

    const SwAttrSet* pSet = GetAttrSet();
    const SvxLineSpacingItem &rSpace = pSet->GetLineSpacing();

    switch( rSpace.GetInterLineSpaceRule() )
    {
        case SVX_INTER_LINE_SPACE_PROP:
        {
            // OD 07.01.2004 #i11859#
            if ( _bNoPropLineSpace )
            {
                break;
            }

            // OD 2004-03-17 #i11860# - use method GetHeightOfLastLine()
            nRet = GetHeightOfLastLine();

            long nTmp = nRet;
            nTmp *= rSpace.GetPropLineSpace();
            nTmp /= 100;
            nTmp -= nRet;
            if ( nTmp > 0 )
                nRet = nTmp;
            else
                nRet = 0;
        }
            break;
        case SVX_INTER_LINE_SPACE_FIX:
        {
            if ( rSpace.GetInterLineSpace() > 0 )
                nRet = rSpace.GetInterLineSpace();
        }
            break;
        default:
            break;
    }
    return nRet;
}

sal_uInt16 SwTextFrm::FirstLineHeight() const
{
    if ( !HasPara() )
    {
        if( IsEmpty() && IsValid() )
            return IsVertical() ? (sal_uInt16)Prt().Width() : (sal_uInt16)Prt().Height();
        return USHRT_MAX;
    }
    const SwParaPortion *pPara = GetPara();
    if ( !pPara )
        return USHRT_MAX;

    return pPara->Height();
}

sal_uInt16 SwTextFrm::GetLineCount( sal_Int32 nPos )
{
    sal_uInt16 nRet = 0;
    SwTextFrm *pFrm = this;
    do
    {
        pFrm->GetFormatted();
        if( !pFrm->HasPara() )
            break;
        SwTextSizeInfo aInf( pFrm );
        SwTextMargin aLine( pFrm, &aInf );
        if( COMPLETE_STRING == nPos )
            aLine.Bottom();
        else
            aLine.CharToLine( nPos );
        nRet = nRet + aLine.GetLineNr();
        pFrm = pFrm->GetFollow();
    } while ( pFrm && pFrm->GetOfst() <= nPos );
    return nRet;
}

void SwTextFrm::ChgThisLines()
{
    // not necessary to format here (GerFormatted etc.), because we have to come from there!
    sal_uLong nNew = 0;
    const SwLineNumberInfo &rInf = GetNode()->GetDoc()->GetLineNumberInfo();
    if ( !GetText().isEmpty() && HasPara() )
    {
        SwTextSizeInfo aInf( this );
        SwTextMargin aLine( this, &aInf );
        if ( rInf.IsCountBlankLines() )
        {
            aLine.Bottom();
            nNew = (sal_uLong)aLine.GetLineNr();
        }
        else
        {
            do
            {
                if( aLine.GetCurr()->HasContent() )
                    ++nNew;
            } while ( aLine.NextLine() );
        }
    }
    else if ( rInf.IsCountBlankLines() )
        nNew = 1;

    if ( nNew != nThisLines )
    {
        if ( !IsInTab() && GetAttrSet()->GetLineNumber().IsCount() )
        {
            nAllLines -= nThisLines;
            nThisLines = nNew;
            nAllLines  += nThisLines;
            SwFrm *pNxt = GetNextContentFrm();
            while( pNxt && pNxt->IsInTab() )
            {
                if( 0 != (pNxt = pNxt->FindTabFrm()) )
                    pNxt = pNxt->FindNextCnt();
            }
            if( pNxt )
                pNxt->InvalidateLineNum();

            // Extend repaint to the bottom.
            if ( HasPara() )
            {
                SwRepaint& rRepaint = GetPara()->GetRepaint();
                rRepaint.Bottom( std::max( rRepaint.Bottom(),
                                       Frm().Top()+Prt().Bottom()));
            }
        }
        else // Paragraphs which are not counted should not manipulate the AllLines.
            nThisLines = nNew;
    }
}

void SwTextFrm::RecalcAllLines()
{
    ValidateLineNum();

    const SwAttrSet *pAttrSet = GetAttrSet();

    if ( !IsInTab() )
    {
        const sal_uLong nOld = GetAllLines();
        const SwFormatLineNumber &rLineNum = pAttrSet->GetLineNumber();
        sal_uLong nNewNum;
        const bool bRestart = GetTextNode()->GetDoc()->GetLineNumberInfo().IsRestartEachPage();

        if ( !IsFollow() && rLineNum.GetStartValue() && rLineNum.IsCount() )
            nNewNum = rLineNum.GetStartValue() - 1;
        // If it is a follow or not has not be considered if it is a restart at each page; the
        // restart should also take effect at follows.
        else if ( bRestart && FindPageFrm()->FindFirstBodyContent() == this )
        {
            nNewNum = 0;
        }
        else
        {
            SwContentFrm *pPrv = GetPrevContentFrm();
            while ( pPrv &&
                    (pPrv->IsInTab() || pPrv->IsInDocBody() != IsInDocBody()) )
                pPrv = pPrv->GetPrevContentFrm();

            // #i78254# Restart line numbering at page change
            // First body content may be in table!
            if ( bRestart && pPrv && pPrv->FindPageFrm() != FindPageFrm() )
                pPrv = 0;

            nNewNum = pPrv ? static_cast<SwTextFrm*>(pPrv)->GetAllLines() : 0;
        }
        if ( rLineNum.IsCount() )
            nNewNum += GetThisLines();

        if ( nOld != nNewNum )
        {
            nAllLines = nNewNum;
            SwContentFrm *pNxt = GetNextContentFrm();
            while ( pNxt &&
                    (pNxt->IsInTab() || pNxt->IsInDocBody() != IsInDocBody()) )
                pNxt = pNxt->GetNextContentFrm();
            if ( pNxt )
            {
                if ( pNxt->GetUpper() != GetUpper() )
                    pNxt->InvalidateLineNum();
                else
                    pNxt->_InvalidateLineNum();
            }
        }
    }
}

void SwTextFrm::VisitPortions( SwPortionHandler& rPH ) const
{
    const SwParaPortion* pPara = IsValid() ? GetPara() : NULL;

    if (pPara)
    {
        if ( IsFollow() )
            rPH.Skip( GetOfst() );

        const SwLineLayout* pLine = pPara;
        while ( pLine )
        {
            const SwLinePortion* pPor = pLine->GetFirstPortion();
            while ( pPor )
            {
                pPor->HandlePortion( rPH );
                pPor = pPor->GetPortion();
            }

            rPH.LineBreak(pLine->Width());
            pLine = pLine->GetNext();
        }
    }

    rPH.Finish();
}

const SwScriptInfo* SwTextFrm::GetScriptInfo() const
{
    const SwParaPortion* pPara = GetPara();
    return pPara ? &pPara->GetScriptInfo() : 0;
}

/**
 * Helper function for SwTextFrm::CalcBasePosForFly()
 */
static SwTwips lcl_CalcFlyBasePos( const SwTextFrm& rFrm, SwRect aFlyRect,
                            SwTextFly& rTextFly )
{
    SWRECTFN( (&rFrm) )
    SwTwips nRet = rFrm.IsRightToLeft() ?
                   (rFrm.Frm().*fnRect->fnGetRight)() :
                   (rFrm.Frm().*fnRect->fnGetLeft)();

    do
    {
        SwRect aRect = rTextFly.GetFrm( aFlyRect );
        if ( 0 != (aRect.*fnRect->fnGetWidth)() )
        {
            if ( rFrm.IsRightToLeft() )
            {
                if ( (aRect.*fnRect->fnGetRight)() -
                     (aFlyRect.*fnRect->fnGetRight)() >= 0 )
                {
                    (aFlyRect.*fnRect->fnSetRight)(
                        (aRect.*fnRect->fnGetLeft)() );
                    nRet = (aRect.*fnRect->fnGetLeft)();
                }
                else
                    break;
            }
            else
            {
                if ( (aFlyRect.*fnRect->fnGetLeft)() -
                     (aRect.*fnRect->fnGetLeft)() >= 0 )
                {
                    (aFlyRect.*fnRect->fnSetLeft)(
                        (aRect.*fnRect->fnGetRight)() + 1 );
                    nRet = (aRect.*fnRect->fnGetRight)();
                }
                else
                    break;
            }
        }
        else
            break;
    }
    while ( (aFlyRect.*fnRect->fnGetWidth)() > 0 );

    return nRet;
}

void SwTextFrm::CalcBaseOfstForFly()
{
    OSL_ENSURE( !IsVertical() || !IsSwapped(),
            "SwTextFrm::CalcBasePosForFly with swapped frame!" );

    const SwNode* pNode = GetTextNode();
    if ( !pNode->getIDocumentSettingAccess()->get(DocumentSettingId::ADD_FLY_OFFSETS) )
        return;

    SWRECTFN( this )

    SwRect aFlyRect( Frm().Pos() + Prt().Pos(), Prt().SSize() );

    // Get first 'real' line and adjust position and height of line rectangle
    // OD 08.09.2003 #110978#, #108749#, #110354# - correct behaviour,
    // if no 'real' line exists (empty paragraph with and without a dummy portion)
    {
        SwTwips nTop = (aFlyRect.*fnRect->fnGetTop)();
        const SwLineLayout* pLay = GetPara();
        SwTwips nLineHeight = 200;
        while( pLay && pLay->IsDummy() && pLay->GetNext() )
        {
            nTop += pLay->Height();
            pLay = pLay->GetNext();
        }
        if ( pLay )
        {
            nLineHeight = pLay->Height();
        }
        (aFlyRect.*fnRect->fnSetTopAndHeight)( nTop, nLineHeight );
    }

    SwTextFly aTextFly( this );
    aTextFly.SetIgnoreCurrentFrame( true );
    aTextFly.SetIgnoreContour( true );
    // #118809# - ignore objects in page header|footer for
    // text frames not in page header|footer
    aTextFly.SetIgnoreObjsInHeaderFooter( true );
    SwTwips nRet1 = lcl_CalcFlyBasePos( *this, aFlyRect, aTextFly );
    aTextFly.SetIgnoreCurrentFrame( false );
    SwTwips nRet2 = lcl_CalcFlyBasePos( *this, aFlyRect, aTextFly );

    // make values relative to frame start position
    SwTwips nLeft = IsRightToLeft() ?
                    (Frm().*fnRect->fnGetRight)() :
                    (Frm().*fnRect->fnGetLeft)();

    mnFlyAnchorOfst = nRet1 - nLeft;
    mnFlyAnchorOfstNoWrap = nRet2 - nLeft;
}

/**
 * Repaint all text frames of the given text node
 */
void SwTextFrm::repaintTextFrames( const SwTextNode& rNode )
{
    SwIterator<SwTextFrm,SwTextNode> aIter( rNode );
    for( const SwTextFrm *pFrm = aIter.First(); pFrm; pFrm = aIter.Next() )
    {
        SwRect aRec( pFrm->PaintArea() );
        const SwRootFrm *pRootFrm = pFrm->getRootFrm();
        SwViewShell *pCurShell = pRootFrm ? pRootFrm->GetCurrShell() : NULL;
        if( pCurShell )
            pCurShell->InvalidateWindows( aRec );
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
