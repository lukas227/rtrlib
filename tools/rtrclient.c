/*
 * This file is part of RTRlib.
 *
 * RTRlib is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * RTRlib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RTRlib; see the file COPYING.LESSER.
 *
 * written by Fabian Holler, in cooperation with:
 * INET group, Hamburg University of Applied Sciences,
 * CST group, Freie Universitaet Berlin
 * Website: http://rpki.realmv6.org/
 */

#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include "rtrlib/rtrlib.h"


void print_usage(char** argv){
    printf("Usage:\n");
    printf(" %s tcp <host> <port>\n", argv[0]);
#ifdef RTRLIB_HAVE_LIBSSH
    printf(" %s ssh <host> <port> <username> <private_key> <public_key> \n", argv[0]);
#endif
    printf("\nExamples:\n");
    printf(" %s tcp 10.10.10.1 42420\n", argv[0]);
#ifdef RTRLIB_HAVE_LIBSSH
    printf(" %s ssh 10.10.10.1 22 rtr_user ~/.ssh/id_rsa ~/.ssh/id_rsa.pub\n", argv[0]);
#endif

}

void ipaddr_to_string(const ip_addr* addr, char* result, const size_t len){
    if(addr->ver == IPV4){
        struct in_addr ina;
        ina.s_addr = addr->u.addr4.addr;
        if(inet_ntop(AF_INET, &ina, result, len) == NULL){
            printf("inet_ntop error\n");
            exit(EXIT_FAILURE);
        }
    }
    else if(addr->ver == IPV6){
        if(inet_ntop(AF_INET6, (struct in6_addr*) &(addr->u.addr6), result, len) == NULL){
            printf("inet_ntop error\n");
            exit(EXIT_FAILURE);
        }
    }
}

void state_cb(const rtr_socket* sock, const rtr_socket_state state){
    printf("Socket State: ");
    switch(state)
    {
        case RTR_CLOSED:
            printf("RTR_CLOSED\n");
            break;
        case RTR_ESTABLISHED:
            printf("RTR_ESTABLISHED\n");
            break;
        case RTR_RESET:
            printf("RTR_RESET\n");
            break;
        case RTR_SYNC:
            printf("RTR_SYNC\n");
            break;
        case RTR_ERROR_NO_DATA_AVAIL:
            printf("RTR_ERROR_NO_DATA_AVAIL\n");
            break;
        case RTR_ERROR_NO_INCR_UPDATE_AVAIL:
            printf("RTR_ERROR_NO_INCR_UPDATE_AVAIL\n");
            break;
        case RTR_ERROR_FATAL:
            printf("RTR_ERROR_FATAL\n");
            break;
        case RTR_ERROR_TRANSPORT:
            printf("RTR_ERROR_TRANSPORT\n");
            break;
        case RTR_SHUTDOWN:
            printf("RTR_SHUTDOWN\n");
            break;
    }
}

void update_cb(struct pfx_table* p, const pfx_record rec, const pfxv_state state){
    char ip[INET6_ADDRSTRLEN];
    ipaddr_to_string(&(rec.prefix), ip, INET6_ADDRSTRLEN);
    printf("state: ");
    if(state == BGP_PFXV_STATE_VALID)
        printf("BGP_PFXV_STATE_VALID\n");
    else if(state == BGP_PFXV_STATE_NOT_FOUND)
        printf("BGP_PFXV_STATE_NOT_FOUND\n");
    else if(state == BGP_PFXV_STATE_INVALID)
        printf("BGP_PFXV_STATE_INVALID\n");
    printf("prefix: %s\n", ip);
    printf("asn: %u\n", rec.asn);
    printf("min_len: %u\n", rec.min_len);
    printf("max_len: %u\n", rec.max_len);
    printf("------------------------------\n\n");
    return;
}


int main(int argc, char** argv){
    enum mode_t { TCP, SSH } mode;
    char* host;
    char* port;
    char* user;
    char* privkey;
    char* pubkey;
    if(argc == 1){
        print_usage(argv);
        return(EXIT_FAILURE);
    }

    if(strncasecmp(argv[1], "tcp", strlen(argv[1])) == 0){
        if(argc != 4){
            print_usage(argv);
            return(EXIT_FAILURE);
        }
        mode = TCP;
        host = argv[2];
        port = argv[3];

    }
    else if(strncasecmp(argv[1], "ssh", strlen(argv[1])) == 0){
#ifndef RTRLIB_HAVE_LIBSSH
        print_usage(argv);
        return(EXIT_FAILURE);
#endif
        if(argc != 7){
            print_usage(argv);
            return(EXIT_FAILURE);
        }

        mode = SSH;
        host = argv[2];
        port = argv[3];
        user = argv[4];
        privkey = argv[5];
        pubkey = argv[6];
    }
    else{
        print_usage(argv);
        return(EXIT_FAILURE);
    }

    tr_socket* tr_sock;
    if(mode == TCP){
        tr_tcp_config config = {
            host,
            port,
        };
        tr_tcp_init(&config, &tr_sock);
    }
#ifdef RTRLIB_HAVE_LIBSSH
    else{
        unsigned int iport = atoi(port);
        tr_ssh_config config = {
            host,
            iport,
            user,
            NULL,
            privkey,
            pubkey
        };
    tr_ssh_init(&config, &tr_sock);
    }
#endif

    pfx_table pfxt;
    rtr_update_fp update_fps[1];
    update_fps[0] = &update_cb;
    pfx_table_init(&pfxt, update_fps, 1);

    struct rtr_socket rtr;

    rtr_connection_state_fp con_state_fps[1];
    update_fps[0] = &update_cb;
    con_state_fps[0] = &state_cb;
    rtr_init(&rtr, tr_sock, &pfxt, 240, 480, con_state_fps, 1);

    pthread_t thrd;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if(pthread_create(&thrd, &attr, (void * (*)(void *)) &rtr_start, &rtr) != 0){
        printf("pthread error\n");
        exit(EXIT_FAILURE);
    }

    pthread_attr_destroy(&attr);
    pthread_join(thrd, NULL);

    return(EXIT_SUCCESS);
}