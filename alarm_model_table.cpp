/**
* Project Clearwater - IMS in the Cloud
* Copyright (C) 2014 Metaswitch Networks Ltd
*
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation, either version 3 of the License, or (at your
* option) any later version, along with the "Special Exception" for use of
* the program along with SSL, set forth below. This program is distributed
* in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more
* details. You should have received a copy of the GNU General Public
* License along with this program. If not, see
* <http://www.gnu.org/licenses/>.
*
* The author can be reached by email at clearwater@metaswitch.com or by
* post at Metaswitch Networks Ltd, 100 Church St, Enfield EN2 6BQ, UK
*
* Special Exception
* Metaswitch Networks Ltd grants you permission to copy, modify,
* propagate, and distribute a work formed by combining OpenSSL with The
* Software, or a work derivative of such a combination, even if such
* copying, modification, propagation, or distribution would otherwise
* violate the terms of the GPL. You must comply with the GPL in all
* respects for all of the code used other than OpenSSL.
* "OpenSSL" means OpenSSL toolkit software distributed by the OpenSSL
* Project and licensed under the OpenSSL Licenses, or a work based on such
* software and licensed under the OpenSSL Licenses.
* "OpenSSL Licenses" means the OpenSSL License and Original SSLeay License
* under which the OpenSSL Project distributes the OpenSSL toolkit software,
* as those licenses appear in the file LICENSE-OPENSSL.
*/

/*
 *  Note: this file originally auto-generated by mib2c using
 *        : mib2c.array-user.conf 15997 2007-03-25 22:28:35Z dts12 $
 *
 *  $Id:$
 */

#include "alarm_model_table.hpp"

static netsnmp_handler_registration* my_handler = NULL;
static netsnmp_table_array_callbacks cb;

oid alarmModelTable_oid[] = { alarmModelTable_TABLE_OID };
size_t alarmModelTable_oid_len = OID_LENGTH(alarmModelTable_oid);

static oid zero_dot_zero_oid[] = { ZERO_DOT_ZERO_OID };
static oid alarm_active_state_oid[] = { ALARM_ACTIVE_STATE_OID };
static oid alarm_clear_state_oid[] = { ALARM_CLEAR_STATE_OID };
static oid itu_alarm_table_row_oid[] = { ITU_ALARM_TABLE_ROW_OID };

/************************************************************
 *
 *  Initializes the alarmModelTable module
 */
void init_alarmModelTable(void)
{
  AlarmTableDefs& defs = AlarmTableDefs::get_instance();

  if (initialize_table_alarmModelTable() == SNMP_ERR_NOERROR)
  {
    for (AlarmTableDefsIterator it = defs.begin(); it != defs.end(); it++)
    {
      alarmModelTable_context* ctx = alarmModelTable_create_row_context((char*) "", 
                                                                        it->index(), 
                                                                        it->state());
      if (ctx)
      {
        ctx->_alarm_table_def = &(*it);

        CONTAINER_INSERT(cb.container, ctx);
      }
    }
  }
}

/************************************************************
 *
 *  Initialize the alarmModelTable table by defining its contents
 *  and how it's structured
 */
int initialize_table_alarmModelTable(void)
{
  netsnmp_table_registration_info *table_info;

  if (my_handler)
  {
    snmp_log(LOG_ERR, "initialize_table_alarmModelTable called again");
    return SNMP_ERR_NOERROR;
  }

  memset(&cb, 0x00, sizeof(cb));

  /** create the table structure itself */
  table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);

  my_handler = netsnmp_create_handler_registration("alarmModelTable",
                                                   netsnmp_table_array_helper_handler,
                                                   alarmModelTable_oid,
                                                   alarmModelTable_oid_len,
                                                   HANDLER_CAN_RONLY);
            
  if (!my_handler || !table_info)
  {
    snmp_log(LOG_ERR, "malloc failed: initialize_table_alarmModelTable");
    return SNMP_ERR_GENERR;
  }

  /*
   * Setting up the table's definition
   */

  /** index: alarmListName */
  netsnmp_table_helper_add_index(table_info, ASN_OCTET_STR);
  /** index: alarmModelIndex */
  netsnmp_table_helper_add_index(table_info, ASN_UNSIGNED);
  /** index: alarmModelState */
  netsnmp_table_helper_add_index(table_info, ASN_UNSIGNED);

  table_info->min_column = alarmModelTable_COL_MIN;
  table_info->max_column = alarmModelTable_COL_MAX;

  /*
   * registering the table with the master agent
   */
  cb.get_value = alarmModelTable_get_value;
  cb.container = netsnmp_container_find("alarmModelTable_primary:"
                                        "alarmModelTable:"
                                        "table_container");
  cb.can_set = 0;

  DEBUGMSGTL(("initialize_table_alarmModelTable", "Registering as table array\n"));

  netsnmp_table_container_register(my_handler, table_info, &cb, cb.container, 1);

  return SNMP_ERR_NOERROR;
}

/************************************************************
 *
 *  This routine is called for get requests to copy the data
 *  from the context to the varbind for the request.
 */
int alarmModelTable_get_value(netsnmp_request_info* request,
                              netsnmp_index* item,
                              netsnmp_table_request_info* table_info)
{
  netsnmp_variable_list* var = request->requestvb;
  alarmModelTable_context* context = (alarmModelTable_context*) item;

  switch(table_info->colnum)
  {
    case COLUMN_ALARMMODELNOTIFICATIONID:
    {
      if (context->_alarm_table_def->severity() == AlarmDef::CLEARED)
      {
        snmp_set_var_typed_value(var, ASN_OBJECT_ID,
                                 (u_char*) alarm_clear_state_oid,
                                 sizeof(alarm_clear_state_oid));
      }
      else
      {
        snmp_set_var_typed_value(var, ASN_OBJECT_ID,
                                 (u_char*) alarm_active_state_oid,
                                 sizeof(alarm_active_state_oid));
      }
    }
    break;
    
    case COLUMN_ALARMMODELVARBINDINDEX:
    {
      static unsigned long var_bind_index = 0;
      snmp_set_var_typed_value(var, ASN_UNSIGNED,
                               (u_char*) &var_bind_index,
                               sizeof(var_bind_index));
    }
    break;
    
    case COLUMN_ALARMMODELVARBINDVALUE:
    {
      static unsigned long var_bind_value = 0;
      snmp_set_var_typed_value(var, ASN_INTEGER,
                               (u_char*) &var_bind_value,
                               sizeof(var_bind_value));
    }
    break;
    
    case COLUMN_ALARMMODELDESCRIPTION:
    {
      snmp_set_var_typed_value(var, ASN_OCTET_STR,
                               (u_char*) context->_alarm_table_def->description().c_str(),
                               context->_alarm_table_def->description().length());
    }
    break;
    
    case COLUMN_ALARMMODELSPECIFICPOINTER:
    {
      snmp_set_var_typed_value(var, ASN_OBJECT_ID,
                               (u_char*) itu_alarm_table_row_oid,
                               sizeof(itu_alarm_table_row_oid));

      var->val.objid[ITUALARMTABLEROW_INDEX] = context->_alarm_table_def->index();
      var->val.objid[ITUALARMTABLEROW_SEVERITY] = context->_alarm_table_def->severity();
    }
    break;
    
    case COLUMN_ALARMMODELVARBINDSUBTREE:
    {
      snmp_set_var_typed_value(var, ASN_OBJECT_ID,
                               (u_char*) zero_dot_zero_oid,
                               sizeof(zero_dot_zero_oid));
    }
    break;
    
    case COLUMN_ALARMMODELRESOURCEPREFIX:
    {
      snmp_set_var_typed_value(var, ASN_OBJECT_ID,
                               (u_char*) zero_dot_zero_oid,
                               sizeof(zero_dot_zero_oid));
    }
    break;
    
    case COLUMN_ALARMMODELROWSTATUS:
    {
      static long row_status = ROWSTATUS_ACTIVE;
      snmp_set_var_typed_value(var, ASN_INTEGER,
                               (u_char*) &row_status,
                               sizeof(row_status));
    }
    break;
    
    default: /** We shouldn't get here */
    {
      snmp_log(LOG_ERR, "unknown column: alarmModelTable_get_value");
      return SNMP_ERR_GENERR;
    }
  }

  return SNMP_ERR_NOERROR;
}

/************************************************************
 * 
 *  Create a new row context and initialize its oid index.
 */
alarmModelTable_context* alarmModelTable_create_row_context(char* name,
                                                            unsigned long index,
                                                            unsigned long state)
{
  alarmModelTable_context* ctx = SNMP_MALLOC_TYPEDEF(alarmModelTable_context);
  if (!ctx)
  {
    snmp_log(LOG_ERR, "malloc failed: alarmModelTable_create_row_context");
    return NULL;
  }
        
  if (alarmModelTable_index_to_oid(name, index, state, &ctx->_index) != SNMP_ERR_NOERROR)
  {
    if (ctx->_index.oids != NULL)
    {
      free(ctx->_index.oids);
    }

    free(ctx);
    return NULL;
  }

  return ctx;
}

/************************************************************
 *
 *  Convert table index components to an oid.
 */
int alarmModelTable_index_to_oid(char* name,
                                 unsigned long index,
                                 unsigned long state,
                                 netsnmp_index *oid_idx)
{
  int err = SNMP_ERR_NOERROR;

  netsnmp_variable_list var_alarmListName;
  netsnmp_variable_list var_alarmModelIndex;
  netsnmp_variable_list var_alarmModelState;

  /*
   * set up varbinds
   */
  memset(&var_alarmListName, 0x00, sizeof(var_alarmListName));
  var_alarmListName.type = ASN_OCTET_STR;
  memset(&var_alarmModelIndex, 0x00, sizeof(var_alarmModelIndex));
  var_alarmModelIndex.type = ASN_UNSIGNED;
  memset(&var_alarmModelState, 0x00, sizeof(var_alarmModelState));
  var_alarmModelState.type = ASN_UNSIGNED;

  /*
   * chain index varbinds together
   */
  var_alarmListName.next_variable = &var_alarmModelIndex;
  var_alarmModelIndex.next_variable = &var_alarmModelState;
  var_alarmModelState.next_variable =  NULL;


  DEBUGMSGTL(("verbose:alarmModelTable:alarmModelTable_index_to_oid", "called\n"));

  snmp_set_var_value(&var_alarmListName, (u_char*) name, strlen(name));
  snmp_set_var_value(&var_alarmModelIndex, (u_char*) &index, sizeof(index));
  snmp_set_var_value(&var_alarmModelState, (u_char*) &state, sizeof(state));

  err = build_oid(&oid_idx->oids, &oid_idx->len, NULL, 0, &var_alarmListName);
  if (err)
  {
    snmp_log(LOG_ERR, "error %d converting index to oid: alarmModelTable_index_to_oid", err);
  }

  /*
   * parsing may have allocated memory. free it.
   */
  snmp_reset_var_buffers(&var_alarmListName);

  return err;
} 

