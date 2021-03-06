/*********************************************************************
 *
 * (C) Copyright Broadcom Corporation 2013-2014
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 **********************************************************************
 *
 * \file         example_bst.c
 *
 * \brief        OpenNSL example program for showing statistics
 *
 * \details      OpenNSL example program for displaying and clearing BST
 *               statistics of a given interface.
 *
 **********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sal/commdefs.h>
#include <opennsl/error.h>
#include <opennsl/cosq.h>
#include <opennsl/vlan.h>
#include <opennsl/switch.h>
#include <sal/driver.h>

#define DEFAULT_VLAN 1
#define MAX_COUNTERS 4
#define MAX_COSQ_COUNT 8
#define SWITCH_UNIT_NUM 0
#define MAX_DIGITS_IN_CHOICE 5

char example_usage[] =
"Syntax: example_bst                                                   \n\r"
"                                                                      \n\r"
"Paramaters: None                                                      \n\r"
"                                                                      \n\r"
"Example: The following command is used to see the bst stats of a port \n\r"
"         example_bst                                                  \n\r"
"                                                                      \n\r"
"Usage Guidelines: This program request the user to enter the port     \n\r"
"                  number interactively                                \n\r";

/*****************************************************************//**
 * \brief Include all ports to default vlan
 *
 * \param unit   [IN]    unit number
 *
 * \return OPENNSL_E_XXX     OpenNSL API return code
 ********************************************************************/
int example_switch_default_vlan_config(int unit)
{
  opennsl_port_config_t pcfg;
  int rv;

  /*
   * Create VLAN with id DEFAULT_VLAN and
   * add ethernet ports to the VLAN
   */
  rv = opennsl_port_config_get(unit, &pcfg);
  if (rv != OPENNSL_E_NONE) {
    printf("Failed to get port configuration. Error %s\n", opennsl_errmsg(rv));
    return rv;
  }

  rv = opennsl_vlan_port_add(unit, DEFAULT_VLAN, pcfg.e, pcfg.e);
  if (rv != OPENNSL_E_NONE) {
    printf("Failed to add ports to VLAN. Error %s\n", opennsl_errmsg(rv));
    return rv;
  }

  return OPENNSL_E_NONE;
}

/**************************************************************************//**
 * \brief   Read numeric menu choice from user.
 *
 * \param   choice         [IN/OUT] choice
 *
 * \return  OPENNSL_E_xxx  OpenNSL API return code
 *****************************************************************************/
int example_read_user_choice(int *choice)
{
    char val;
    char digits[MAX_DIGITS_IN_CHOICE + 1];
    int idx = 0;
    int valid = TRUE;

    /* parse input string until \n */
    while((val = getchar()) != '\n')
    {
        if ((val >= '0' && val <= '9') && idx < MAX_DIGITS_IN_CHOICE)
        {
            digits[idx++] = val;
        }
        else
        {
            valid = FALSE;
        }
    }
    if ((valid == TRUE) && idx != 0)
    {
        digits[idx] = '\0';
        *choice = atoi(digits);
        return(OPENNSL_E_NONE);
    }
    else
    {
        *choice = -1;
        return(OPENNSL_E_FAIL);
    }
}

typedef struct {
  opennsl_bst_stat_id_t bid;
  char name[50];
} example_bst_counter_t;


/*****************************************************************//**
 * \brief Main function for bst sample application
 *
 * \param argc, argv         commands line arguments
 *
 * \return OPENNSL_E_XXX     OpenNSL API return code
 ********************************************************************/
/* Main function for BST application */
int main(int argc, char *argv[])
{
  opennsl_error_t   rc;
  int choice;
  int unit = SWITCH_UNIT_NUM;
  int index;
  opennsl_gport_t gport;
  opennsl_port_t port;
  opennsl_cos_queue_t cosq;
  uint32 options = 0;
  example_bst_counter_t id_list[MAX_COUNTERS] = {
    {opennslBstStatIdUcast,            "opennslBstStatIdUcast"},
    {opennslBstStatIdMcast,            "opennslBstStatIdMcast"},
    {opennslBstStatIdPriGroupShared,   "opennslBstStatIdPriGroupShared"},
    {opennslBstStatIdPriGroupHeadroom, "opennslBstStatIdPriGroupHeadroom"}
  };
  uint64 val[MAX_COUNTERS][MAX_COSQ_COUNT] = {0};

  if ((argc != 1) || ((argc > 1) && (strcmp(argv[1], "--help") == 0))) {
    printf("%s\n\r", example_usage);
    return OPENNSL_E_PARAM;
  }

  /* Initialize the system. */
  printf("Initializing the system.\r\n");
  rc = opennsl_driver_init();

  if (rc != OPENNSL_E_NONE) {
    printf("\r\nFailed to initialize the system, rc = %d (%s).\r\n",
           rc, opennsl_errmsg(rc));
    return 0;
  }

  /* Add ports to default vlan. */
  printf("Adding ports to default vlan.\r\n");
  rc = example_switch_default_vlan_config(unit);
  if (rc != OPENNSL_E_NONE) {
    printf("\r\nFailed to add default ports, rc = %d (%s).\r\n",
           rc, opennsl_errmsg(rc));
  }

  /* Enabling bst module. */
  rc = opennsl_switch_control_set(unit, opennslSwitchBstEnable, 1);
  if (rc != OPENNSL_E_NONE) {
    printf("\r\nFailed to Enable bst, rc = %d (%s).\r\n",
           rc, opennsl_errmsg(rc));
    return 0;
  }
  printf("BST feature is enabled.\r\n");


  while (1) {
    printf("\nUser Menu: Select one of the following options\n");
    printf("1. Display bst statistics of a port.\n");
    printf("2. Clear bst statistics of a port.\n");
    printf("9. Launch diagnostic shell\n");
    printf("0. Quit the application.\n");

    if(example_read_user_choice(&choice) != OPENNSL_E_NONE)
    {
        printf("Invalid option entered. Please re-enter.\n");
        continue;
    }
    switch(choice) {

      case 1:
      {
        printf("\r\nEnter the port number.\r\n");
        if(example_read_user_choice(&port) != OPENNSL_E_NONE)
        {
            printf("Invalid option entered. Please re-enter.\n");
            continue;
        }
        rc = opennsl_port_gport_get (unit, port, &gport);
        if (rc != OPENNSL_E_NONE)
        {
          return OPENNSL_E_FAIL;
        }
        for (index = 0; index < MAX_COUNTERS; index++)
        {
          rc = (opennsl_cosq_bst_stat_sync(unit, id_list[index].bid));
          if (rc != OPENNSL_E_NONE) {
            printf("\r\nFailed to sync the state of port, rc = %d (%s).\r\n",
                   rc, opennsl_errmsg(rc));
            break;
          }
        }

        for (index = 0; index < MAX_COUNTERS; index++)
        {
          for (cosq = 0; cosq <=7; cosq++)
          {
            rc = opennsl_cosq_bst_stat_get(unit, gport, cosq,
                id_list[index].bid, options, &val[index][cosq]);
            if (rc != OPENNSL_E_NONE) {
              printf("\r\nFailed to get the port stats, rc = %d (%s).\r\n",
                     rc, opennsl_errmsg(rc));
              break;
            }
            printf("BST Counter: %s for COS queue: %d is : %llu\n",
                id_list[index].name, cosq, val[index][cosq]);
          }
          printf("\n");
        }

        break;
      } /* End of case 1 */

      case 2:
      {
        printf("\r\nEnter the port number.\r\n");
        if(example_read_user_choice(&port) != OPENNSL_E_NONE)
        {
            printf("Invalid option entered. Please re-enter.\n");
            continue;
        }
        rc = opennsl_port_gport_get (unit, port, &gport);
        if (rc!= OPENNSL_E_NONE)
        {
          return OPENNSL_E_FAIL;
        }

        for (index = 0; index < MAX_COUNTERS; index++)
        {
          for (cosq = 0; cosq <= 7; cosq++)
          {
            rc = opennsl_cosq_bst_stat_clear(unit, gport, cosq,
                id_list[index].bid);
            if (rc != OPENNSL_E_NONE) {
              printf("\r\nFailed to clear the port stats, rc = %d (%s).\r\n",
                     rc, opennsl_errmsg(rc));
              break;
            }
          }
          printf("\n");
        }

        printf("\r\nPort %d stats cleared\r\n", port);
        break;
      } /* End of case 2 */

      case 9:
      {
        opennsl_driver_shell();
        break;
      }
      case 0:
      {
        printf("Exiting the application.\n");
        return OPENNSL_E_NONE;
      }

      default :
        break;
    } /* End of switch */
  } /* End of while */

  return 0;
}

