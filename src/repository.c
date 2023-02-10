#include <clientname/certificate.h>
#include <clientname/modem.h>
#include <clientname/repository.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/socket.h>

#define LOG_MODULE_NAME repository
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* repository thread */
K_THREAD_STACK_DEFINE(repository_stack, CONFIG_REPOSITORY_STACK_SIZE);
static struct k_thread repository_tcb;
static k_tid_t repository_thread_id;

K_MSGQ_DEFINE(out_msgq, sizeof(union message), CONFIG_REPOSITORY_MESSAGE_COUNT,
              8);

/* buffer size of incoming and outgoing data */
static char recv_buf[CONFIG_REPOSITORY_BUFFER_SIZE] = {0};
// static char send_buf[CONFIG_REPOSITORY_BUFFER_SIZE] = {0};

/* incoming messages come from here */
static char in_path[CONFIG_REPOSITORY_PATH_LENGTH] = {0};

/* outgoing messages sent to here*/
static char out_path[CONFIG_REPOSITORY_PATH_LENGTH] = {0};

/* wait time between get requests */
static uint32_t wait_time = 5;

ssize_t request(char *buf, size_t buf_len, const char *path) {
  return snprintf(buf, buf_len,
                  "GET %s HTTP/1.1\r\n"
                  "Connection: keep-alive\r\n"
                  "Host: " CONFIG_REPOSITORY_HOSTNAME
                  ":" STR(CONFIG_REPOSITORY_PORT) "\r\n\r\n",
                  path);
}

static void response_cb(struct http_response *rsp,
                        enum http_final_call final_data, void *user_data) {
  if (final_data == HTTP_DATA_MORE) {
    LOG_INF("Partial data received (%zd bytes)", rsp->data_len);
  } else if (final_data == HTTP_DATA_FINAL) {
    LOG_INF("All the data received (%zd bytes)", rsp->data_len);
  }

  LOG_INF("Response status %s", rsp->http_status);
  LOG_INF("Response data %s", rsp->recv_buf);
  if (rsp->http_status_code == 204) {
    LOG_INF("Response data %s", rsp->recv_buf);
  }
}

static void repository_thread(void *a, void *b, void *c) {
  /* response from getaddr */
  struct addrinfo *res = NULL;

  /* hints for socket setup */
  struct addrinfo hints = {
      .ai_family = AF_INET,
      .ai_socktype = SOCK_STREAM,
  };

  /* result error */
  int err = 0;
  /* file descripter */
  int fd = 0;

  LOG_INF("incoming messages from:\r\n\t%s", out_path);
  LOG_INF("outgoing messages to:\r\n\t%s", in_path);

  /* Main repository loop */
  while (1) {
    /* Do clean up */
    if (fd > 0) {
      close(fd);
    }
    if (res != NULL) {
      freeaddrinfo(res);
      res = NULL;
    }

    /* Handle DNS */
    do {
      err = getaddrinfo(CONFIG_REPOSITORY_HOSTNAME, STR(CONFIG_REPOSITORY_PORT),
                        &hints, &res);
      if (err) {
        LOG_WRN("getaddrinfo() failed, err %d\n", errno);
        /* if we did not the DNS then wait 1 second */
        k_sleep(K_SECONDS(1));
      } else {
        uint8_t *addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr.s4_addr;
        LOG_INF("-> %u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
      }
    } while (err != 0); /* End handling DNS */

    /* Setup sockets */
    ((struct sockaddr_in *)res->ai_addr)->sin_port =
        htons(CONFIG_REPOSITORY_PORT);

    k_sleep(K_SECONDS(5));
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);
    LOG_INF("errno %d", errno);
    if (fd == -1) {
      LOG_ERR("Failed to open socket!");
      /* free address*/
      continue;
    }

    /* Setup TLS socket options */
    err = tls_setup(fd);
    if (err) {
      LOG_ERR("Failed to initialize TLS");
      continue;
    }

    /* try to connect */
    int retry = 0;
    do {
      err = connect(fd, res->ai_addr, sizeof(struct sockaddr_in));
      if (err) {
        LOG_ERR("connect() failed, err: %d", errno);
        retry++;
      }
      /* if we retry too many times then give up and redo everything */
      if (retry > 4) {
        LOG_ERR("Couldn't connect, giving up");
        break;
      }
    } while (err);

    /* if not connected then redo everything */
    if (err) {
      LOG_ERR("connect() did not succeed, err: %d", errno);
      continue;
    }

    /* Main work loop */
    while (1) {
      k_sleep(K_SECONDS(wait_time));
      struct http_request req = {0};
      req.method = HTTP_GET;
      req.url = out_path;
      req.host = CONFIG_REPOSITORY_HOSTNAME;
      req.protocol = "HTTP/1.1";
      req.response = response_cb;
      req.recv_buf = recv_buf;
      req.recv_buf_len = sizeof(recv_buf);

      /* sock is a file descriptor referencing a socket that has been connected
       * to the HTTP server.
       */
      int ret = http_client_req(fd, &req, 5000, NULL);
      LOG_INF("http_client_req() returned %d", ret);
      if (ret < 0) {
        LOG_ERR("http_client_req() failed, err: %d", errno);
        break;
      }
    } /* End main work loop */

  } /* End Main repository loop */
}

void repository_send_message(union message msg) {
  while (k_msgq_put(&out_msgq, &msg, K_NO_WAIT) != 0) {
    k_msgq_purge(&out_msgq);
  }
}

void repository_init() {
  char id[40] = {0};

  /* setup gateway id*/
  modem_get_uuid(id, sizeof(id));

  /* build outgoing path */
  snprintf(in_path, CONFIG_REPOSITORY_PATH_LENGTH, "/gateway/%s/in", id);

  /* build incoming path */
  snprintf(out_path, CONFIG_REPOSITORY_PATH_LENGTH, "/gateway/%s/out", id);

  repository_thread_id =
      k_thread_create(&repository_tcb, repository_stack,
                      K_THREAD_STACK_SIZEOF(repository_stack),
                      repository_thread, NULL, NULL, NULL, 0, 0, K_NO_WAIT);
}