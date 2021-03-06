/************************************************************************
 * Copyright(c) 2009, One Unified. All rights reserved.                 *
 * email: info@oneunified.net                                           *
 *                                                                      *
 * This file is provided as is WITHOUT ANY WARRANTY                     *
 *  without even the implied warranty of                                *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                *
 *                                                                      *
 * This software may not be used nor distributed without proper license *
 * agreement.                                                           *
 *                                                                      *
 * See the file LICENSE.txt for redistribution information.             *
 ************************************************************************/

//#include "StdAfx.h"

#include <iostream>

#include <OUCommon/TimeSource.h>

#include "IQFeedSymbol.h"

namespace ou { // One Unified
namespace tf { // TradeFrame

IQFeedSymbol::IQFeedSymbol(const symbol_id_t& sSymbol, pInstrument_t pInstrument) 
: 
  Symbol<IQFeedSymbol>( pInstrument, sSymbol ),
  m_cnt( 0 ), m_dblTrade( 0 ), m_dblChange( 0 ), m_nTradeSize( 0 ), m_nTotalVolume( ),
  m_dblBid( 0 ), m_dblAsk( 0 ), m_nBidSize( 0 ), m_nAskSize( 0 ), 
  m_dblOpen( 0 ), m_dblClose( 0 ), m_cntTrades( 0 ), m_dblHigh( 0 ), m_dblLow( 0 ), 
  m_nShortInterest( 0 ), m_dblPriceEarnings( 0 ), m_dbl52WkHi( 0 ), m_dbl52WkLo( 0 ), m_dblDividendYield( 0.0 ),
  m_nOpenInterest( 0 ), m_QStatus( qUnknown ),
  m_bQuoteTradeWatchInProgress( false ), m_bDepthWatchInProgress( false )
{
}

IQFeedSymbol::~IQFeedSymbol(void) {
}

void IQFeedSymbol::HandleFundamentalMessage( IQFFundamentalMessage *pMsg ) {
  m_sOptionRoots = pMsg->Field( IQFFundamentalMessage::FRootOptionSymbols );
  m_AverageVolume = pMsg->Integer( IQFFundamentalMessage::FAveVolume );
  m_sCompanyName = pMsg->Field( IQFFundamentalMessage::FName );
  m_Precision = pMsg->Integer( IQFFundamentalMessage::FPrecision );
  m_dblHistoricalVolatility = pMsg->Double( IQFFundamentalMessage::FVolatility );
  m_dblStrikePrice = pMsg->Double( IQFFundamentalMessage::FStrikePrice );
  m_nShortInterest = pMsg->Integer( IQFFundamentalMessage::FShortInterest );
  m_dblPriceEarnings = pMsg->Double( IQFFundamentalMessage::FPE );
  m_dbl52WkHi = pMsg->Double( IQFFundamentalMessage::F52WkHi );
  m_dbl52WkLo = pMsg->Double( IQFFundamentalMessage::F52WkLo );
  m_dblDividendAmount = pMsg->Double( IQFFundamentalMessage::FDivAmt );
  m_dblDividendRate = pMsg->Double( IQFFundamentalMessage::FDivRate );
  m_dblDividendYield = pMsg->Double( IQFFundamentalMessage::FDivYld );
  m_dateExDividend = pMsg->Date( IQFFundamentalMessage::FDivExDate );

  OnFundamentalMessage( *this );
}

template <typename T>
void IQFeedSymbol::DecodePricingMessage( IQFPricingMessage<T> *pMsg ) {
  m_bNewTrade = m_bNewQuote = m_bNewOpen = false;
  std::string sLastTradeTime = pMsg->Field( IQFPricingMessage<T>::QPLastTradeTime );
  if ( sLastTradeTime.length() > 0 ) {  // can we do 'assume' anything if it is 0?
    ptime dtLastTrade;
    double dblOpen, dblBid, dblAsk;
    int nBidSize, nAskSize;
    char chType = sLastTradeTime[ sLastTradeTime.length() - 1 ];
    m_dtLastTrade = pMsg->LastTradeTime();
    switch ( chType ) {
    case 't':
    case 'T':
      m_dblTrade = pMsg->Double( IQFPricingMessage<T>::QPLast );
      m_dblChange = pMsg->Double( IQFPricingMessage<T>::QPChange );
      m_nTotalVolume = pMsg->Integer( IQFPricingMessage<T>::QPTtlVol );
      m_nTradeSize = pMsg->Integer( IQFPricingMessage<T>::QPLastVol );
      m_dblHigh = pMsg->Double( IQFPricingMessage<T>::QPHigh );
      m_dblLow = pMsg->Double( IQFPricingMessage<T>::QPLow );
      m_dblClose = pMsg->Double( IQFPricingMessage<T>::QPClose );
      m_cntTrades = pMsg->Integer( IQFPricingMessage<T>::QPNumTrades );
      m_bNewTrade = true;

      dblOpen = pMsg->Double( IQFPricingMessage<T>::QPOpen );
      if ( ( m_dblOpen != dblOpen ) && ( 0 != dblOpen ) ) { 
        m_dblOpen = dblOpen; 
        m_bNewOpen = true; 
        std::cout << "IQF new open: " << GetId() << "=" << m_dblOpen << std::endl;
      };
      m_nOpenInterest = pMsg->Integer( IQFPricingMessage<T>::QPOpenInterest );

      // fall through to processing bid / ask
    case 'b':
    case 'a':
      dblBid = pMsg->Double( IQFPricingMessage<T>::QPBid );
      if ( m_dblBid != dblBid ) { m_dblBid = dblBid; m_bNewQuote = true; }
      nBidSize = pMsg->Integer( IQFPricingMessage<T>::QPBidSize );
      if ( m_nBidSize != nBidSize ) { m_nBidSize = nBidSize; m_bNewQuote = true; }
      dblAsk = pMsg->Double( IQFPricingMessage<T>::QPAsk );
      if ( m_dblAsk != dblAsk ) { m_dblAsk = dblAsk; m_bNewQuote = true; }
      nAskSize = pMsg->Integer( IQFPricingMessage<T>::QPAskSize );
      if ( m_nAskSize != nAskSize ) { m_nAskSize = nAskSize; m_bNewQuote = true; }
      break;
    case 'o':
      break;
    }
  }

  //CString s;
  //s.Format( "%s: %c %0.2f@%d b=%0.2f@%d a=%0.2f@%d #=%d", 
  //  m_sSymbol, chType, dblTrade, TradeSize, dblBid, BidSize, dblAsk, AskSize, cntTrades );
  //theApp.pConsoleMessages->WriteLine( s ); 
}

void IQFeedSymbol::HandleSummaryMessage( IQFSummaryMessage *pMsg ) {
  DecodePricingMessage<IQFSummaryMessage>( pMsg );
  OnSummaryMessage( *this );
}

void IQFeedSymbol::HandleUpdateMessage( IQFUpdateMessage *pMsg ) {

  if ( qUnknown == m_QStatus ) {
    m_QStatus = ( "Not Found" == pMsg->Field( IQFPricingMessage<IQFUpdateMessage>::QPLast ) ) ? qNotFound : qFound;
    if ( qNotFound == m_QStatus ) {
      std::cout << GetId() << " not found" << std::endl;
    }
  }
  if ( qFound == m_QStatus ) {
    DecodePricingMessage<IQFUpdateMessage>( pMsg );
    OnUpdateMessage( *this );
    //ptime dt( microsec_clock::local_time() );
    ptime dt( ou::TimeSource::Instance().External() );
    // quote needs to be sent before the trade
    if ( m_bNewQuote ) {
      Quote quote( dt, m_dblBid, m_nBidSize, m_dblAsk, m_nAskSize );
      Symbol::m_OnQuote( quote );
    }
    if ( m_bNewTrade ) {
      Trade trade( dt, m_dblTrade, m_nTradeSize );
      Symbol::m_OnTrade( trade );
      if ( m_bNewOpen ) {
        Symbol::m_OnOpen( trade );
      }
    }
  }
}

void IQFeedSymbol::HandleNewsMessage( IQFNewsMessage *pMsg ) {
}

} // namespace tf
} // namespace ou
