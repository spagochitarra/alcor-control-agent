// c includes
#include "aca_comm_mgr.h"
#include "aca_log.h"
#include "aca_util.h"
#include "goalstate.pb.h"
#include "trn_rpc_protocol.h"
#include <unistd.h> /* for getopt */
#include <chrono>
#include <string.h>
#include <thread>

#define ACALOGNAME "AliothControlAgent"

using namespace std;
using aca_comm_manager::Aca_Comm_Manager;

// Global variables
char *g_rpc_server = NULL;
char *g_rpc_protocol = NULL;
bool g_debug_mode = false;

using std::string;

static void aca_cleanup()
{
    // Optional:  Delete all global objects allocated by libprotobuf.
    google::protobuf::ShutdownProtobufLibrary();

    aca_free(g_rpc_server);
    aca_free(g_rpc_protocol);

    ACA_LOG_INFO("Program exiting, cleaning up...\n");
    ACA_LOG_CLOSE();
}

// function to handle ctrl-c and kill process
static void aca_signal_handler(int sig_num)
{
    fprintf(stdout, "Caught signal: %d\n", sig_num);

    // perform all the necessary cleanup here
    aca_cleanup();

    exit(sig_num);
}

int main(int argc, char *argv[])
{
    int option;
    int rc;
    ACA_LOG_INIT(ACALOGNAME);

    // Register the signal handlers
    signal(SIGINT, aca_signal_handler);
    signal(SIGTERM, aca_signal_handler);

    while ((option = getopt(argc, argv, "s:p:d")) != -1)
    {
        switch (option)
        {
        case 's':
            g_rpc_server = (char *)malloc(sizeof(optarg));
            if (g_rpc_server != NULL)
            {
                strncpy(g_rpc_server, optarg, strlen(optarg) + 1);
            }
            else
            {
                ACA_LOG_EMERG("Out of memory when allocating string with size: %lu.\n",
                              sizeof(optarg));
                exit(EXIT_FAILURE);
            }
            break;
        case 'p':
            g_rpc_protocol = (char *)malloc(sizeof(optarg));
            if (g_rpc_protocol != NULL)
            {
                strncpy(g_rpc_protocol, optarg, strlen(optarg) + 1);
            }
            else
            {
                ACA_LOG_EMERG("Out of memory when allocating string with size: %lu.\n",
                              sizeof(optarg));
                exit(EXIT_FAILURE);
            }
            break;
        case 'd':
            g_debug_mode = true;
            break;
        default: /* the '?' case when the option is not recognized */
            fprintf(stderr,
                    "Usage: %s\n"
                    "\t\t[-s transitd RPC server]\n"
                    "\t\t[-p transitd RPC protocol]\n"
                    "\t\t[-d enable debug mode]\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // fill in the RPC server and protocol if it is not provided in command line arg
    if (g_rpc_server == NULL)
    {
        g_rpc_server = (char *)malloc(sizeof(LOCALHOST));
        if (g_rpc_server != NULL)
        {
            strncpy(g_rpc_server, LOCALHOST, strlen(LOCALHOST) + 1);
        }
        else
        {
            ACA_LOG_EMERG("Out of memory when allocating string with size: %lu.\n",
                          sizeof(LOCALHOST));
            exit(EXIT_FAILURE);
        }
    }
    if (g_rpc_protocol == NULL)
    {
        g_rpc_protocol = (char *)malloc(sizeof(UDP));
        if (g_rpc_protocol != NULL)
        {
            strncpy(g_rpc_protocol, UDP, strlen(UDP) + 1);
        }
        else
        {
            ACA_LOG_EMERG("Out of memory when allocating string with size: %lu.\n",
                          sizeof(UDP));
            exit(EXIT_FAILURE);
        }
    }

    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    aliothcontroller::GoalState GoalState_builder;
    aliothcontroller::PortState *new_port_states =
        GoalState_builder.add_port_states();
    aliothcontroller::SubnetState *new_subnet_states =
        GoalState_builder.add_subnet_states();
    aliothcontroller::VpcState *new_vpc_states =
        GoalState_builder.add_vpc_states();

    // fill in port state structs

    new_port_states->set_operation_type(aliothcontroller::OperationType::CREATE);

    // this will allocate new PortConfiguration, will need to free it later
    aliothcontroller::PortConfiguration *PortConfiguration_builder =
        new_port_states->mutable_configuration();
    PortConfiguration_builder->set_version(1);
    PortConfiguration_builder->set_project_id(
        "dbf72700-5106-4a7a-918f-111111111111");
    PortConfiguration_builder->set_network_id("2");
    PortConfiguration_builder->set_id("dd12d1dadad2g4h");
    PortConfiguration_builder->set_name("Peer1");
    PortConfiguration_builder->set_admin_state_up(true);
    PortConfiguration_builder->set_mac_address("fa:16:3e:d7:f2:6c");
    PortConfiguration_builder->set_veth_name("veth0");
    PortConfiguration_builder->set_host_ip("172.0.0.2");
    // this will allocate new PortConfiguration_FixedIp will need to free later
    aliothcontroller::PortConfiguration_FixedIp *PortIp_builder =
        PortConfiguration_builder->add_fixed_ips();
    PortIp_builder->set_ip_address("10.0.0.2");
    PortIp_builder->set_subnet_id("2");
    // this will allocate new PortConfiguration_SecurityGroupId will need to free later
    aliothcontroller::PortConfiguration_SecurityGroupId *SecurityGroup_builder =
        PortConfiguration_builder->add_security_group_ids();
    SecurityGroup_builder->set_id("1");
    // this will allocate new PortConfiguration_AllowAddressPair will need to free later
    aliothcontroller::PortConfiguration_AllowAddressPair
        *AddressPair_builder =
            PortConfiguration_builder->add_allow_address_pairs();
    AddressPair_builder->set_ip_address("10.0.0.5");
    AddressPair_builder->set_mac_address("fa:16:3e:d7:f2:9f");
    // this will allocate new PortConfiguration_ExtraDhcpOption will need to free later
    aliothcontroller::PortConfiguration_ExtraDhcpOption *ExtraDhcp_builder =
        PortConfiguration_builder->add_extra_dhcp_options();
    ExtraDhcp_builder->set_name("opt_1");
    ExtraDhcp_builder->set_value("12");

    // fill in the subnet state structs

    new_subnet_states->set_operation_type(aliothcontroller::OperationType::INFO);

    // this will allocate new SubnetConfiguration, need to free it later
    aliothcontroller::SubnetConfiguration *SubnetConiguration_builder =
        new_subnet_states->mutable_configuration();
    SubnetConiguration_builder->set_version(1);
    SubnetConiguration_builder->set_project_id(
        "dbf72700-5106-4a7a-918f-111111111111");
    // VpcConiguration_builder->set_id("99d9d709-8478-4b46-9f3f-2206b1023fd3");
    SubnetConiguration_builder->set_vpc_id("99d9d709-8478-4b46-9f3f-2206b1023fd3");
    SubnetConiguration_builder->set_id("2");
    SubnetConiguration_builder->set_name("SuperSubnet");
    SubnetConiguration_builder->set_cidr("10.0.0.1/16");
    SubnetConiguration_builder->set_tunnel_id(22222);
    // this will allocate new SubnetConfiguration_TransitSwitchIp, may to free it later
    aliothcontroller::SubnetConfiguration_TransitSwitchIp *TransitSwitchIp_builder =
        SubnetConiguration_builder->add_transit_switch_ips();
    TransitSwitchIp_builder->set_ip_address("172.0.0.1");

    // fill in the vpc state structs

    // this will allocate new VpcConfiguration, need to free it later

    new_vpc_states->set_operation_type(aliothcontroller::OperationType::CREATE_UPDATE_SWITCH);

    aliothcontroller::VpcConfiguration *VpcConiguration_builder =
        new_vpc_states->mutable_configuration();
    VpcConiguration_builder->set_version(1);
    VpcConiguration_builder->set_project_id(
        "dbf72700-5106-4a7a-918f-a016853911f8");
    // VpcConiguration_builder->set_id("99d9d709-8478-4b46-9f3f-2206b1023fd3");
    VpcConiguration_builder->set_id("1");
    VpcConiguration_builder->set_name("SuperVpc");
    VpcConiguration_builder->set_cidr("192.168.0.0/24");
    VpcConiguration_builder->set_tunnel_id(11111);
    // this will allocate new VpcConfiguration_TransitRouterIp, may to free it later
    aliothcontroller::VpcConfiguration_TransitRouterIp *TransitRouterIp_builder =
        VpcConiguration_builder->add_transit_router_ips();
    TransitRouterIp_builder->set_vpc_id("12345");
    TransitRouterIp_builder->set_ip_address("10.0.0.2");

    string string_message;

    // Serialize it to string
    GoalState_builder.SerializeToString(&string_message);
    fprintf(stdout, "(NOTE USED) Serialized protobuf string: %s\n",
            string_message.c_str());

    // Serialize it to binary array
    size_t size = GoalState_builder.ByteSize();
    char *buffer = (char *)malloc(size);
    GoalState_builder.SerializeToArray(buffer, size);
    string binary_message(buffer, size);
    fprintf(stdout, "Serialized protobuf binary array: %s\n",
            binary_message.c_str());

    aliothcontroller::GoalState parsed_struct;

    rc = Aca_Comm_Manager::get_instance().deserialize(binary_message, parsed_struct);

    aca_free(buffer);

    if (rc == EXIT_SUCCESS)
    {

        fprintf(stdout, "Deserialize succeed, comparing the content now...\n");

        assert(parsed_struct.port_states_size() ==
               GoalState_builder.port_states_size());
        for (int i = 0; i < parsed_struct.port_states_size(); i++)
        {
            assert(parsed_struct.port_states(i).operation_type() ==
                   GoalState_builder.port_states(i).operation_type());

            assert(parsed_struct.port_states(i).configuration().version() ==
                   GoalState_builder.port_states(i).configuration().version());

            assert(parsed_struct.port_states(i).configuration().project_id() ==
                   GoalState_builder.port_states(i).configuration().project_id());

            assert(parsed_struct.port_states(i).configuration().network_id() ==
                   GoalState_builder.port_states(i).configuration().network_id());

            assert(parsed_struct.port_states(i).configuration().id() ==
                   GoalState_builder.port_states(i).configuration().id());

            assert(parsed_struct.port_states(i).configuration().name() ==
                   GoalState_builder.port_states(i).configuration().name());

            assert(parsed_struct.port_states(i).configuration().name() ==
                   GoalState_builder.port_states(i).configuration().name());

            assert(parsed_struct.port_states(i).configuration().admin_state_up() ==
                   GoalState_builder.port_states(i).configuration().admin_state_up());

            assert(parsed_struct.port_states(i).configuration().mac_address() ==
                   GoalState_builder.port_states(i).configuration().mac_address());

            assert(parsed_struct.port_states(i).configuration().veth_name() ==
                   GoalState_builder.port_states(i).configuration().veth_name());

            assert(parsed_struct.port_states(i).configuration().host_ip() ==
                   GoalState_builder.port_states(i).configuration().host_ip());

            assert(parsed_struct.port_states(i).configuration().fixed_ips_size() ==
                   GoalState_builder.port_states(i).configuration().fixed_ips_size());
            for (int j = 0; j < parsed_struct.port_states(i).configuration().fixed_ips_size(); j++)
            {
                assert(parsed_struct.port_states(i).configuration().fixed_ips(j).subnet_id() ==
                       GoalState_builder.port_states(i).configuration().fixed_ips(j).subnet_id());

                assert(parsed_struct.port_states(i).configuration().fixed_ips(j).ip_address() ==
                       GoalState_builder.port_states(i).configuration().fixed_ips(j).ip_address());
            }

            assert(parsed_struct.port_states(i).configuration().security_group_ids_size() ==
                   GoalState_builder.port_states(i).configuration().security_group_ids_size());
            for (int j = 0; j < parsed_struct.port_states(i).configuration().security_group_ids_size(); j++)
            {
                assert(parsed_struct.port_states(i).configuration().security_group_ids(j).id() ==
                       GoalState_builder.port_states(i).configuration().security_group_ids(j).id());
            }

            assert(parsed_struct.port_states(i).configuration().allow_address_pairs_size() ==
                   GoalState_builder.port_states(i).configuration().allow_address_pairs_size());
            for (int j = 0; j < parsed_struct.port_states(i).configuration().allow_address_pairs_size(); j++)
            {
                assert(parsed_struct.port_states(i).configuration().allow_address_pairs(j).ip_address() ==
                       GoalState_builder.port_states(i).configuration().allow_address_pairs(j).ip_address());

                assert(parsed_struct.port_states(i).configuration().allow_address_pairs(j).mac_address() ==
                       GoalState_builder.port_states(i).configuration().allow_address_pairs(j).mac_address());
            }

            assert(parsed_struct.port_states(i).configuration().extra_dhcp_options_size() ==
                   GoalState_builder.port_states(i).configuration().extra_dhcp_options_size());
            for (int j = 0; j < parsed_struct.port_states(i).configuration().extra_dhcp_options_size(); j++)
            {
                assert(parsed_struct.port_states(i).configuration().extra_dhcp_options(j).name() ==
                       GoalState_builder.port_states(i).configuration().extra_dhcp_options(j).name());

                assert(parsed_struct.port_states(i).configuration().extra_dhcp_options(j).value() ==
                       GoalState_builder.port_states(i).configuration().extra_dhcp_options(j).value());
            }
        }
        assert(parsed_struct.subnet_states_size() ==
               GoalState_builder.subnet_states_size());

        for (int i = 0; i < parsed_struct.subnet_states_size(); i++)
        {

            assert(parsed_struct.subnet_states(i).operation_type() ==
                   GoalState_builder.subnet_states(i).operation_type());

            assert(parsed_struct.subnet_states(i).configuration().version() ==
                   GoalState_builder.subnet_states(i).configuration().version());

            assert(parsed_struct.subnet_states(i).configuration().project_id() ==
                   GoalState_builder.subnet_states(i).configuration().project_id());

            assert(parsed_struct.subnet_states(i).configuration().vpc_id() ==
                   GoalState_builder.subnet_states(i).configuration().vpc_id());

            assert(parsed_struct.subnet_states(i).configuration().id() ==
                   GoalState_builder.subnet_states(i).configuration().id());

            assert(parsed_struct.subnet_states(i).configuration().name() ==
                   GoalState_builder.subnet_states(i).configuration().name());

            assert(parsed_struct.subnet_states(i).configuration().cidr() ==
                   GoalState_builder.subnet_states(i).configuration().cidr());

            assert(parsed_struct.subnet_states(i).configuration().transit_switch_ips_size() ==
                   GoalState_builder.subnet_states(i).configuration().transit_switch_ips_size());

            for (int j = 0; j < parsed_struct.subnet_states(i).configuration().transit_switch_ips_size(); j++)
            {
                assert(parsed_struct.subnet_states(i).configuration().transit_switch_ips(j).ip_address() ==
                       GoalState_builder.subnet_states(i).configuration().transit_switch_ips(j).ip_address());
            }
        }

        assert(parsed_struct.vpc_states_size() ==
               GoalState_builder.vpc_states_size());

        for (int i = 0; i < parsed_struct.vpc_states_size(); i++)
        {
            assert(parsed_struct.vpc_states(i).operation_type() ==
                   GoalState_builder.vpc_states(i).operation_type());

            assert(parsed_struct.vpc_states(i).configuration().version() ==
                   GoalState_builder.vpc_states(i).configuration().version());

            assert(parsed_struct.vpc_states(i).configuration().project_id() ==
                   GoalState_builder.vpc_states(i).configuration().project_id());

            assert(parsed_struct.vpc_states(i).configuration().id() ==
                   GoalState_builder.vpc_states(i).configuration().id());

            assert(parsed_struct.vpc_states(i).configuration().name() ==
                   GoalState_builder.vpc_states(i).configuration().name());

            assert(parsed_struct.vpc_states(i).configuration().cidr() ==
                   GoalState_builder.vpc_states(i).configuration().cidr());

            assert(parsed_struct.vpc_states(i).configuration().tunnel_id() ==
                   GoalState_builder.vpc_states(i).configuration().tunnel_id());

            assert(parsed_struct.vpc_states(i).configuration().subnet_ids_size() ==
                   GoalState_builder.vpc_states(i).configuration().subnet_ids_size());

            for (int j = 0; j < parsed_struct.vpc_states(i).configuration().subnet_ids_size(); j++)
            {
                assert(parsed_struct.vpc_states(i).configuration().subnet_ids(j).id() ==
                       GoalState_builder.vpc_states(i).configuration().subnet_ids(j).id());
            }

            assert(parsed_struct.vpc_states(i).configuration().routes_size() ==
                   GoalState_builder.vpc_states(i).configuration().routes_size());

            for (int k = 0; k < parsed_struct.vpc_states(i).configuration().routes_size(); k++)
            {
                assert(parsed_struct.vpc_states(i).configuration().routes(k).destination() ==
                       GoalState_builder.vpc_states(i).configuration().routes(k).destination());

                assert(parsed_struct.vpc_states(i).configuration().routes(k).next_hop() ==
                       GoalState_builder.vpc_states(i).configuration().routes(k).next_hop());
            }

            assert(parsed_struct.vpc_states(i).configuration().transit_router_ips_size() ==
                   GoalState_builder.vpc_states(i).configuration().transit_router_ips_size());

            for (int l = 0; l < parsed_struct.vpc_states(i).configuration().transit_router_ips_size(); l++)
            {
                assert(parsed_struct.vpc_states(i).configuration().transit_router_ips(l).vpc_id() ==
                       GoalState_builder.vpc_states(i).configuration().transit_router_ips(l).vpc_id());

                assert(parsed_struct.vpc_states(i).configuration().transit_router_ips(l).ip_address() ==
                       GoalState_builder.vpc_states(i).configuration().transit_router_ips(l).ip_address());
            }
        }

        fprintf(stdout, "All content matched, send the parsed_struct to update_goal_state...\n");

        int rc = Aca_Comm_Manager::get_instance().update_goal_state(parsed_struct);
        if (rc == EXIT_SUCCESS)
        {
            fprintf(stdout, "[Functional test] Successfully executed the network controller command");
        }
        else
        {
            fprintf(stdout,
                    "[Funtional test] Unable to execute the network controller command: %d\n", rc);
        }
    }
    else
    {
        fprintf(stdout, "Deserialize failed with error code: %u\n", rc);
    }
    // free the allocated VpcConfiguration since we are done with it now
    new_port_states->clear_configuration();
    new_subnet_states->clear_configuration();
    new_vpc_states->clear_configuration();

    aca_cleanup();

    return rc;
}