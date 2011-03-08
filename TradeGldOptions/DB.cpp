/************************************************************************
 * Copyright(c) 2011, One Unified. All rights reserved.                 *
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

#include "StdAfx.h"

#include <stdexcept>

#define BOOST_FILESYSTEM_VERSION 3
#include <boost/filesystem.hpp>

#include "DB.h"

using namespace ou::tf;

std::string CDB::m_sPortfolioId( "dn01" );

CDB::CDB(void): m_bOpened( false ) {
}

CDB::~CDB(void) {
  Close();
}

void CDB::Open( const std::string& sDbName ) {

  if ( !m_bOpened ) {
    if ( boost::filesystem::exists( sDbName ) ) {
      // open already created and loaded database
      m_session.Open( sDbName );
      ou::tf::db::RegisterRowDefinitions( m_session );
    }
    else {
      // create and build new database
      m_session.Open( sDbName, ou::db::EOpenFlagsAutoCreate );
      ou::tf::db::RegisterTableCreation( m_session );
      m_session.CreateTables();
      ou::tf::db::RegisterRowDefinitions( m_session );
      ou::tf::db::PopulateTables( m_session );
      Populate();
    }
    m_bOpened = true;
  }

}

void CDB::Close( void ) {
  if ( m_bOpened ) {
    m_bOpened = false;
    m_session.Close();
  }
}

void CDB::Populate( void ) {

  CAccountAdvisor::TableRowDef aa( "ray", "Raymond Burkholder", "One Unified" );
  ou::db::QueryFields<CAccountAdvisor::TableRowDef>::pQueryFields_t paa = m_session.Insert<CAccountAdvisor::TableRowDef>( aa );

  CAccountOwner::TableRowDef ao( "ray", "ray", "Raymond", "Burkholder" );
  ou::db::QueryFields<CAccountOwner::TableRowDef>::pQueryFields_t pao = m_session.Insert<CAccountOwner::TableRowDef>( ao );

  CAccount::TableRowDef acctIB( "ib01", "ray", "Raymond Burkholder", keytypes::EProviderIB, "Interactive Brokers", "acctid", "login", "password" );
  ou::db::QueryFields<CAccount::TableRowDef>::pQueryFields_t paIB = m_session.Insert<CAccount::TableRowDef>( acctIB );

  CAccount::TableRowDef acctIQ( "iq01", "ray", "Raymond Burkholder", keytypes::EProviderIQF, "IQFeed", "acctid", "login", "password" );
  ou::db::QueryFields<CAccount::TableRowDef>::pQueryFields_t paIQF = m_session.Insert<CAccount::TableRowDef>( acctIQ );

  CPortfolio::TableRowDef portfolio( PortfolioId(), "ray", "Delta Neutral 01 - long und, long put" );
  ou::db::QueryFields<CPortfolio::TableRowDef>::pQueryFields_t pPortfolio = m_session.Insert<CPortfolio::TableRowDef>( portfolio );

}

struct PortfolioQuery { // can this be simplified?
  template<class A>
  void Fields( A& a ) {
    ou::db::Field( a, "portfolioid", idPortfolio );
  }
  const ou::tf::keytypes::idPortfolio_t& idPortfolio;
  //PortfolioQuery( void ) {};
  PortfolioQuery( const ou::tf::keytypes::idPortfolio_t& idPortfolio_ ) : idPortfolio( idPortfolio_ ) {};
};

void CDB::LoadPortfolio( const ou::tf::keytypes::idPortfolio_t& id, CPortfolio::pPortfolio_t& pPortfolio ) {

  PortfolioQuery query( id );
  
  CPortfolio::TableRowDef portfolio;  // can we put stuff directly into object?
  ou::db::QueryFields<PortfolioQuery>::pQueryFields_t pQuery 
    = m_session.SQL<PortfolioQuery>( "select * from portfolios", query ).Where( "portfolioid = ?" ).NoExecute();

  m_session.Bind<PortfolioQuery>( pQuery );
  if ( m_session.Execute( pQuery ) ) {
    m_session.Columns<PortfolioQuery, CPortfolio::TableRowDef>( pQuery, portfolio );
  }
  else {
    throw std::runtime_error( "no portfolio found" );
  }

  pPortfolio.reset( new CPortfolio( portfolio ) );

}

struct UnderlyingQuery {  // can this be simplified like PorfolioQuery?
  template<class A>
  void Fields( A& a ) {
    ou::db::Field( a, "instrumentid", idInstrument );
  }
  const ou::tf::keytypes::idInstrument_t& idInstrument;
  //UnderlyingQuery( void ) {};
  UnderlyingQuery( const ou::tf::keytypes::idInstrument_t& idInstrument_ ) : idInstrument( idInstrument_ ) {};
};

bool CDB::LoadUnderlying( const ou::tf::keytypes::idInstrument_t& id, ou::tf::CInstrument::pInstrument_t& pInstrument ) {

  bool bFound = false;
  UnderlyingQuery query( id );

  CInstrument::TableRowDef instrument;  // can we put stuff direclty into object?
  ou::db::QueryFields<UnderlyingQuery>::pQueryFields_t pQuery 
    = m_session.SQL<UnderlyingQuery>( "select * from instruments", query ).Where( "instrumentid = ?" ).NoExecute();

  m_session.Bind<UnderlyingQuery>( pQuery );
  if ( m_session.Execute( pQuery ) ) {
    m_session.Columns<UnderlyingQuery, CInstrument::TableRowDef>( pQuery, instrument );
    pInstrument.reset( new CInstrument( instrument ) );
    bFound = true;
  }

  return bFound;
}

void CDB::SaveUnderlying( ou::tf::CInstrument::pInstrument_t& pInstrument ) {
  ou::db::QueryFields<CInstrument::TableRowDef>::pQueryFields_t pQuery 
    = m_session.Insert<CInstrument::TableRowDef>( const_cast<CInstrument::TableRowDef&>( pInstrument->GetRow() ) );
}