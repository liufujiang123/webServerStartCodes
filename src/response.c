#include "response.h"
char *my_http_version = "HTTP/1.1";
char *space = " ";
char *crlf = "\r\n";
char *colon = ":";
char *default_path = "./static_site";
char *default_file = "index.html";
char *msg = "\r\n";
int check_method(char *method)
{
  if (!strcmp(method, "GET"))
    return 1;
  if (!strcmp(method, "HEAD"))
    return 2;
  if (!strcmp(method, "POST"))
    return 3;
  return 0;
}
int handle_request(int client_sock, int my_sock, dynamic_buffer *db, struct sockaddr_in client_addr)
{
  // 除了能正常响应的请求，其余都导致断开连接
  Request *request = parse(db->buf, db->current_size, client_sock);
  if (request == NULL)
  {
    // 格式错误
    handle_400(db, client_addr);
    free_request(request);
    return CLOSE;
  }

  if (strcmp(request->http_version, "HTTP/1.1"))
  {
    // printf("HTTP%s", request->http_version);
    handle_505(db, client_addr);
    free_request(request);
    return CLOSE;
  }

  // check connection:close
  // 为正确方法确定返回状态
  Return_value return_value = PERSISTENT;
  char *connection_value = get_header_value(request, "Connection");
  if ((connection_value != NULL) && (!strcmp(connection_value, "Close")))
  {
    // Still --> May be a Post or Get to send msg here.
    return_value = CLOSE_FROM_CLIENT;
  }

  if (check_method(request->http_method) == 1)
  {
    // get
    return_value = handle_get(request, db, client_addr, return_value);
  }
  else if (check_method(request->http_method) == 2)
  {
    // head
    return_value = handle_head(request, db, client_addr, return_value);
  }
  else if (check_method(request->http_method) == 3)
  {
    // post
    return_value = handle_post(request, db, client_addr, return_value);
  }
  else if (!check_method(request->http_method))
  {
    // 不支持的请求
    handle_501(db, client_addr);
    free_request(request);
  }

  return return_value;
}

int handle_get(Request *request, dynamic_buffer *db, struct sockaddr_in client_addr, int return_value)
{
  // 把db修改为将要发送的报文，返回决定是否继续连接的状态
  reset_dynamic_buffer(db);
  dynamic_buffer *url_buf = (dynamic_buffer *)malloc(sizeof(dynamic_buffer *));
  init_dynamic_buffer(url_buf);
  char *end_url;
  if ((end_url = strstr(request->http_uri, "?")) != NULL)
  {
    (*end_url) = 0;
  }

  if (!strcmp(request->http_uri, "/"))
  { // 构造路径
    append_dynamic_buffer(url_buf, default_path, strlen(default_path));
    append_dynamic_buffer(url_buf, request->http_uri, strlen(request->http_uri));
    append_dynamic_buffer(url_buf, default_file, strlen(default_file));
  }
  else
  {
    // 找不到文件
    handle_404(db, client_addr);
    free_request(request);
    free_dynamic_buffer(url_buf);
    // debug
    printf("it's in handle_get 404\n");
    return CLOSE;
  }

  set_response(db, "200", "OK");
  if (return_value == CLOSE)
  {
    set_header(db, "Connection", "Close");
  }
  else
  {
    set_header(db, "Connection", "keep-alive");
  }
  set_msg(db, crlf, strlen(crlf));
  dynamic_buffer *dbuf;
  dbuf = (dynamic_buffer *)malloc(sizeof(dynamic_buffer *));
  init_dynamic_buffer(dbuf);

  // dbuf承接文件内容
  if (!get_file_content(dbuf, url_buf))
  {
    handle_404(db, client_addr);
    free_request(request);
    free_dynamic_buffer(url_buf);
    return CLOSE;
  }
  append_dynamic_buffer(db, dbuf->buf, dbuf->current_size);

  if (return_value != PERSISTENT)
  {
    printf("it's in handle_get's decision\n");
    return CLOSE;
  }

  return PERSISTENT;
}
int get_file_content(dynamic_buffer *dbuf, dynamic_buffer *url_buf)
{
  int fd = open(url_buf->buf, O_RDONLY);
  if (fd < 0)
  {
    // 不能打开
    return 0;
  }
  struct stat file_state;
  if (fstat(fd, &file_state) < 0)
  {
    return 0;
  }
  int file_size = file_state.st_size;
  char *file;
  if ((file = mmap(0, file_size, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED)
  {
    close(fd);
    return 0;
  }
  append_dynamic_buffer(dbuf, file, file_size);
  return 1;
}
int handle_head(Request *request, dynamic_buffer *db, struct sockaddr_in client_addr, int return_value)
{
  reset_dynamic_buffer(db);
  dynamic_buffer *url_buf = (dynamic_buffer *)malloc(sizeof(dynamic_buffer *));
  init_dynamic_buffer(url_buf);
  char *end_url;
  if ((end_url = strstr(request->http_uri, "?")) != NULL)
  {
    (*end_url) = 0;
  }

  if (!strcmp(request->http_uri, "/"))
  { // 构造路径
    append_dynamic_buffer(url_buf, default_path, strlen(default_path));
    append_dynamic_buffer(url_buf, request->http_uri, strlen(request->http_uri));
    append_dynamic_buffer(url_buf, default_file, strlen(default_file));
  }
  else
  {
    // 找不到文件
    handle_404(db, client_addr);
    free_request(request);
    free_dynamic_buffer(url_buf);
    return CLOSE;
  }
  set_response(db, "200", "OK");
  if (return_value == CLOSE)
  {
    set_header(db, "Connection", "Close");
  }
  else
  {
    set_header(db, "Connection", "keep-alive");
  }
  if (return_value != PERSISTENT)
    return CLOSE;
  return PERSISTENT;
}
int handle_post(Request *request, dynamic_buffer *db, struct sockaddr_in client_addr, int return_value)
{
  // 将原请求报文回发
  dynamic_buffer *new_buffer = (dynamic_buffer *)malloc(sizeof(dynamic_buffer *));
  init_dynamic_buffer(new_buffer);
  dynamic_buffer *url_buf = (dynamic_buffer *)malloc(sizeof(dynamic_buffer *));
  init_dynamic_buffer(url_buf);
  char *end_url;
  if ((end_url = strstr(request->http_uri, "?")) != NULL)
  {
    (*end_url) = 0;
  }

  if (!strcmp(request->http_uri, "/"))
  { // 构造路径
    append_dynamic_buffer(url_buf, default_path, strlen(default_path));
    append_dynamic_buffer(url_buf, request->http_uri, strlen(request->http_uri));
    append_dynamic_buffer(url_buf, default_file, strlen(default_file));
  }
  else
  {
    // 找不到文件
    handle_404(db, client_addr);
    free_request(request);
    free_dynamic_buffer(url_buf);
    return CLOSE;
  }
  set_response(new_buffer, "200", "OK");
  if (return_value == CLOSE)
  {
    set_header(new_buffer, "Connection", "Close");
  }
  else
  {
    set_header(new_buffer, "Connection", "keep-alive");
  }
  // 将原报文以及响应头和响应行写入db
  append_dynamic_buffer(new_buffer, db->buf, db->current_size);
  memset_dynamic_buffer(db);
  append_dynamic_buffer(db, new_buffer->buf, new_buffer->current_size);
  free_request(request);

  if (return_value != PERSISTENT)
    return CLOSE;
  return PERSISTENT;
}

// Error Number Handler
void handle_400(dynamic_buffer *dbuf, struct sockaddr_in cli_addr)
{
  memset_dynamic_buffer(dbuf);
  set_response(dbuf, "400", "Bad request");
  set_header(dbuf, "Connection", "Close");
  set_msg(dbuf, crlf, strlen(crlf));
  // ErrorLog("400 Bad request", cli_addr, current_clinet_fd);
  return;
}
void handle_404(dynamic_buffer *dbuf, struct sockaddr_in client_addr)
{
  memset_dynamic_buffer(dbuf);
  set_response(dbuf, "404", "No Found");
  set_header(dbuf, "Connection", "Close");
  set_msg(dbuf, crlf, strlen(crlf));
  return;
}
void handle_501(dynamic_buffer *dbuf, struct sockaddr_in client_addr)
{
  memset_dynamic_buffer(dbuf);
  set_response(dbuf, "501", "Not Implemented");
  set_header(dbuf, "Connection", "Close");
  set_msg(dbuf, crlf, strlen(crlf));
  return;
}
void handle_505(dynamic_buffer *dbuf, struct sockaddr_in client_addr)
{
  memset_dynamic_buffer(dbuf);
  set_response(dbuf, "501", "Version not supported");
  set_header(dbuf, "Connection", "Close");
  set_msg(dbuf, crlf, strlen(crlf));
  return;
}
void set_response(dynamic_buffer *dbuf, char *code, char *description)
{
  append_dynamic_buffer(dbuf, my_http_version, strlen(my_http_version));
  append_dynamic_buffer(dbuf, space, strlen(space));
  append_dynamic_buffer(dbuf, code, strlen(code));
  append_dynamic_buffer(dbuf, space, strlen(space));
  append_dynamic_buffer(dbuf, description, strlen(description));
  append_dynamic_buffer(dbuf, crlf, strlen(crlf));
  return;
}
void set_header(dynamic_buffer *dbuf, char *key, char *value)
{

  append_dynamic_buffer(dbuf, key, strlen(key));
  append_dynamic_buffer(dbuf, colon, strlen(colon));
  append_dynamic_buffer(dbuf, space, strlen(space));
  append_dynamic_buffer(dbuf, value, strlen(value));
  append_dynamic_buffer(dbuf, crlf, strlen(crlf));
  return;
}
void set_msg(dynamic_buffer *dbuf, char *end, int len)
{
  append_dynamic_buffer(dbuf, msg, len);
  return;
}
// Free Request
void free_request(Request *request)
{
  if (request == NULL)
    return;
  free(request->headers);
  request->headers = NULL;
  free(request);
  request = NULL;
  return;
}
char *get_header_value(Request *request, char *header_name)
{
  int i;
  for (i = 0; i < request->header_count; i++)
  {
    if (!strcmp(request->headers[i].header_name, header_name))
      return request->headers[i].header_value;
  }
  return NULL;
}