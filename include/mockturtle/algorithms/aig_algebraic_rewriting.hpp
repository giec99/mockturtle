/*!
  \file aig_algebraic_rewriting.hpp
  \brief AIG algebraric rewriting

  EPFL CS-472 2021 Final Project Option 1
*/

#pragma once

#include "../networks/aig.hpp"
#include "../views/depth_view.hpp"
#include "../views/topo_view.hpp"
#include <vector>

namespace mockturtle
{

namespace detail
{

template<class Ntk>
class aig_algebraic_rewriting_impl
{
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  aig_algebraic_rewriting_impl( Ntk& ntk )
      : ntk( ntk )
  {
    static_assert( has_level_v<Ntk>, "Ntk does not implement depth interface." );
  }

  void run()
  {
    bool cont{ true }; /* continue trying */
    while ( cont )
    {
      cont = false; /* break the loop if no updates can be made */
      ntk.foreach_gate( [&]( node n )
                        {
                          if ( try_algebraic_rules( n ) )
                          {
                            ntk.update_levels();
                            cont = true;
                          }
                        } );
    }
  }

private:
  /* Try various algebraic rules on node n. Return true if the network is updated. */
  bool try_algebraic_rules( node n )
  {
     if ( try_associativity( n ) )
      return true;

    if ( try_distributivity( n ) )
      return true; 
    /* TODO: add more rules here... */

    if ( try_three_level_distributivity( n ) )
      return true; 

    return false;
  }



  bool try_associativity( node n )
  {
    std::vector<signal> children;
    std::vector<signal> to_move;
    bool found = false;
    uint8_t level = 0;
    uint8_t level1 = 0;

    ntk.foreach_fanin( n, [&]( signal child )
                       {
                         if ( !ntk.is_complemented( child ) && ntk.is_on_critical_path( ntk.get_node( child ) ) )
                         {
                           ntk.foreach_fanin( ntk.get_node( child ), [&]( signal sig )
                                              {
                                                if ( ntk.is_on_critical_path( ntk.get_node( sig ) ) )

                                                {
                                                  to_move.push_back( sig );
                                                  found = true;
                                                  // level1 = ntk.level( ntk.get_node( sig ) );
                                                }
                                                else
                                                {
                                                  children.push_back( sig );
                                                }
                                              } );
                         }
                         else
                         {
                           children.push_back( child );
                           level = ntk.level( ntk.get_node( child ) );
                         }
                       } );

    if ( found == true && int( to_move.size() ) == 1 && int( children.size() ) == 2 )

    {

      if ( ntk.level( ntk.get_node( to_move.at( 0 ) ) ) > level )
      {
        signal sub;
        sub = ntk.create_and( to_move.at( 0 ), ntk.create_and( children.at( 0 ), children.at( 1 ) ) );

        ntk.substitute_node( n, sub );

        return true;
      }
    }

    return false;
  }

  /* Try the distributivity rule on node n. Return true if the network is updated. */
  bool try_distributivity( node n )
  {
    uint8_t critical_signals = 0;
    uint8_t counter = 0;
    uint8_t counter2 = 0;
    bool to_apply = false;
    signal critical;
    signal first;
    signal second;

    ntk.foreach_fanin( n, [&]( signal child ) {                                               //check if it is advantageous
      if ( ntk.is_on_critical_path( ntk.get_node( child ) ) && ntk.is_complemented( child ) ) // must be negated and critical
      {
        ntk.foreach_fanin( ntk.get_node( child ), [&]( signal sig )
                           {
                             if ( critical_signals == 0 ) //first time that I check for critical final signals
                             {
                               if ( ntk.is_on_critical_path( ntk.get_node( sig ) ) )
                               {
                                 critical = sig; //problem if both on critical path
                                 counter++;
                               }
                               else
                               {
                                 first = sig;
                               }
                             }
                             else if ( critical_signals == 1 && counter == 1 ) //second time check for critical signal
                             {
                               if ( ntk.is_on_critical_path( ntk.get_node( sig ) ) )
                               {
                                 if ( sig == critical )
                                 {
                                   to_apply = true; //problem if both on critical path
                                 }
                                 counter2++;
                               }
                               else
                               {
                                 second = sig;
                               }
                             }
                           } );
        critical_signals++;
      }

    } );

    if ( to_apply && counter2 == 1 )
    {

      signal sub;
      if ( ntk.is_or( n ) )
      {
        sub = ntk.create_and( critical, ntk.create_or( first, second ) );
      }
      else
      {
        sub = ntk.create_nand( critical, ntk.create_or( first, second ) );
      }
      ntk.substitute_node( n, sub );
      return true;
    }

    return false;
  }
  

  bool try_three_level_distributivity(node n)
  {
    std::vector<signal> children;
    std::vector<signal> to_check;
    std::vector<signal> crit;
    uint8_t level_low;
    uint8_t level_crit;

    

          ntk.foreach_fanin( n, [&]( signal child )
                       {
                         if ( ntk.is_on_critical_path( ntk.get_node( child ) ) && ntk.is_complemented( child ) )
                         {
                           to_check.push_back( child );
                         }
                         else if ( !ntk.is_on_critical_path( ntk.get_node( child ) ) )
                         {
                           children.push_back( child );
                         }
                       } );
            
                      if(int(to_check.size())==1 && int(children.size())==1)
                          {
                                     ntk.foreach_fanin( ntk.get_node( to_check.at( 0 ) ), [&]( signal granchild )
                                     {
                                       if ( ntk.is_on_critical_path( ntk.get_node( granchild ) ) && ntk.is_complemented( granchild ) )
                                       {
                                         to_check.push_back( granchild );
                                       }
                                       else if ( ntk.is_complemented( granchild ) )
                                       {
                                         children.push_back( granchild );
                                       }
                                     } );   
                          }

                    if(int(to_check.size())==2 && int(children.size())==2)
                    {
                            ntk.foreach_fanin( ntk.get_node( to_check.at( 1 ) ), [&]( signal grangranchild )
                         {
                           if ( ntk.is_on_critical_path( ntk.get_node( grangranchild ) ) )
                           {
                             to_check.push_back( grangranchild ); // TODO : double critical path can be problem
                           }
                           else
                           {
                             children.push_back( grangranchild );
                           }
                         } );  
                    }


        if(int(to_check.size())==3 && int(children.size())==3)
        {
            level_low = ntk.level( ntk.get_node(children.at(0) ));
            level_crit  = ntk.level( ntk.get_node(to_check.at(0)) );

            if ( level_crit > 2 + level_low )
                {
                  signal bottom_and;
                  bottom_and = ntk.create_and( children.at( 2 ), children.at( 0 ) );
                  signal right_and;
                  right_and = ntk.create_and( to_check.at( 2 ), bottom_and );
                  signal to_sub;
                  signal left_and;
                  left_and = ntk.create_and( children.at( 0 ), ntk.create_not( children.at( 1 )) );
                  signal right_and_comp;
                  signal left_and_comp;
                  right_and_comp = ntk.create_not( right_and );
                  left_and_comp = ntk.create_not( left_and );
                  signal new_netw;
                  new_netw = ntk.create_nand( right_and_comp , left_and_comp );

                  ntk.substitute_node( n, new_netw );
                    return true;

                }
                   
        }
       
          return false; 
  }



private:
  Ntk& ntk;
};

} // namespace detail

/* Entry point for users to call */
template<class Ntk>
void aig_algebraic_rewriting( Ntk& ntk )
{
  static_assert( std::is_same_v<typename Ntk::base_type, aig_network>, "Ntk is not an AIG" );

  depth_view dntk{ ntk };
  detail::aig_algebraic_rewriting_impl p( dntk );
  p.run();
}

} /* namespace mockturtle */
