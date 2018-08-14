#include "header.h"

#include <time.h>
#include <sys/socket.h>

void handletraffic()
{
	struct eNB_conn_info connection_information =
	{
		.broadcast.port = 0,
		.broadcast.sock = 0,

		.pdcch.port = 0,
		.pdcch.sock = 0,

		.pucch.port = 0,
		.pucch.sock = 0,

		.ul_sch.port = 0,
		.ul_sch.sock = 0,

		.prach.port = 0,
		.prach.sock = 0,

		.srb.port = 0,
		.srb.sock = 0,

		.dl_sch.port = 0,
		.dl_sch.sock = 0	
	};

	struct UE_INFO my_states;

	setup_ue(&my_states);

	int broadcast_sock;
	struct MIB_MESSAGE init_mib_msg;
	setup_broadcast_socket(&broadcast_sock);
	while(my_states.UE_state == 0)
	{
		receive_broadcast(broadcast_sock, &my_states, &init_mib_msg);
	}
	setup_connection_information(&connection_information, init_mib_msg);

	connection_information.broadcast.sock = broadcast_sock;

	const int max_epoll_events = 6;
	struct epoll_event ev = {}, events[max_epoll_events];

	int efd = epoll_create(1);

	open_channels(&connection_information, &ev, &efd);

	int ewait_flag, i;
	struct timespec start, check;

	clock_gettime(CLOCK_REALTIME, &start);

	#ifndef DEBUG
		print_initial_offset();
	#endif
	
	while(1)
	{
		clock_gettime(CLOCK_REALTIME, &check);
		if(my_states.UE_state >= 5)
			handle_drx(&my_states, connection_information.pucch, &start.tv_sec, check.tv_sec);

		ewait_flag = epoll_wait(efd, events, max_epoll_events, -1);
		if(ewait_flag == -1)
		{
			perror("epoll wait ");
			exit(EXIT_FAILURE);
		}

		if(my_states.uplink_resource_grant)
		{
			states_check(&connection_information, &my_states);
			for(i = 0; i < ewait_flag; i++)
			{
				if(events[i].events & EPOLLIN)
				{
					if(events[i].data.fd == connection_information.pdcch.sock)
					{
						#ifdef DEBUG
							printf("\nreceive_dci\n");
						#endif
						receive_dci(events[i].data.fd, &my_states);
					}
					if(events[i].data.fd == connection_information.broadcast.sock)
					{
						#ifdef DEBUG
							printf("\nreceive_broadcast\n");
						#endif
						receive_broadcast(events[i].data.fd, &my_states, &init_mib_msg);
					}

					if((events[i].data.fd == connection_information.dl_sch.sock) 
						&& (my_states.UE_state == RANDOM_ACCESS_PREAMBLE))
					{
						#ifdef DEBUG
							printf("\nreceive_random_access_response\n");
						#endif
						receive_random_access_response(events[i].data.fd, &my_states);
					}

					if((events[i].data.fd == connection_information.dl_sch.sock) 
						&& (my_states.UE_state == RRC_REQUEST))
					{
						#ifdef DEBUG
							printf("\nreceive_rrc_setup\n");
						#endif
						receive_rrc_setup(events[i].data.fd, &my_states);
					}
				}
			}
		}
		else
		{
			#ifdef DEBUG
				printf("dropping packets\n");
			#endif
			drop_packets(connection_information);
		}
		
		#ifndef DEBUG
			print_cell(my_states);
		#endif
		if(my_states.battery_life <= 0)
		{
			printf("I guess I will die ¯\\_(ツ)_/¯\n");
			exit(EXIT_SUCCESS);
		}
	}
}

void states_check(struct eNB_conn_info *connections, struct UE_INFO *info)
{
	if(info->UE_state == RANDOM_ACCESS_PREAMBLE)
	{
		#ifdef DEBUG
			printf("send_random_access_preamble\n");
		#endif
		send_random_access_preamble(connections->prach, info);
	}

	if(info->UE_state == RANDOM_ACCESS_RESPONSE)
	{
		#ifdef DEBUG
			printf("send_rrc_req\n");
		#endif
		send_rrc_req(connections->ul_sch, info);
	}

	if(info->UE_state == RRC_SETUP)
	{
		connections->srb.port = info->srb_identity;
		if(setup_socket(&connections->srb, SOCK_STREAM) == -1)
		{
			#ifdef DEBUG
				printf("Error on setup socket\n");
			#endif
			exit(EXIT_FAILURE);
		}
		info->UE_state = RRC_SETUP_COMPLETE;
	}

	if(info->UE_state == RRC_SETUP_COMPLETE)
	{
		send_rrc_setup_complete(connections->srb, info);
	}
}

void handle_drx(struct UE_INFO *info, struct conn_pair pucch, time_t *start, time_t check)
{
	int drx_timer = 0;
	time_t runtime = check - *start;
	#ifdef DEBUG
		printf("TIME = %ld s\n", runtime);
		printf("drx_cycle_type = %d\n", info->uplink_power_control.drx_cycle_type);
		printf("short_drx_timer = %d\n", info->uplink_power_control.short_drx_timer);
		printf("long_drx_timer = %d\n", info->uplink_power_control.long_drx_timer);
		printf("Battery = %d%%\n", info->battery_life);
	#endif

	if(info->uplink_power_control.drx_cycle_type == 0)
		drx_timer = info->uplink_power_control.short_drx_timer;
	else
		drx_timer = info->uplink_power_control.long_drx_timer;

	if(runtime != 0 && ((runtime % info->uplink_power_control.on_duration_timer) == 0) && info->uplink_resource_grant == 1)
	{
		info->uplink_resource_grant = 0;
		#ifdef DEBUG
			printf("Sending UCI\n");
		#endif
		send_uci(pucch, info);
	}

	if((runtime % drx_timer) == 0 && info->uplink_resource_grant == 0)
	{
		info->uplink_resource_grant = 1;
		*start = check;
		if(info->battery_life >= 10)
			info->battery_life -= 10;
	}
}