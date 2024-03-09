const http = require('http');
const fs = require('fs');
const path = require('path');

const server = http.createServer((req, res) => {
  // 解析请求的URL，如果请求根路径("/")，则默认为请求index.html文件
  const url = req.url === '/' ? '/lab2/index.html' :'/lab2'+ req.url;
  const filePath = path.join(__dirname, url);

  // 检查文件是否存在
  fs.access(filePath, fs.constants.F_OK, (err) => {
    if (err) {
      // 如果文件不存在，返回404错误
      res.writeHead(404, { 'Content-Type': 'text/plain' });
      res.end('404 Not Found');
      return;
    }

    // 获取文件的扩展名，用于设置正确的Content-Type
    const fileExtension = path.extname(filePath);
    let contentType = 'text/html'; // 默认为HTML

    // 根据扩展名设置Content-Type
    switch (fileExtension) {
      case '.html':
        contentType = 'text/html';
        break;
      case '.css':
        contentType = 'text/css';
        break;
      case '.js':
        contentType = 'text/javascript';
        break;
      case '.png':
        contentType = 'image/png';
        break;
      case '.jpg':
      case '.jpeg':
        contentType = 'image/jpeg';
        break;
      case '.mp3':
        contentType = 'audio/mpeg';
        break;
      // 添加其他可能的文件类型
    }


// 根据文件的扩展名决定是否使用'utf8'字符编码
const isBinaryFile = ['.jpg', '.jpeg', '.png', '.mp3'].includes(fileExtension);

// 读取文件并发送响应
fs.readFile(filePath, isBinaryFile ? null : 'utf8', (err, data) => {
  if (err) {
    // 如果读取文件出错，返回500错误
    res.writeHead(500, { 'Content-Type': 'text/plain' });
    res.end('Internal Server Error');
    return;
  }

  // 设置响应的Content-Type
  res.writeHead(200, { 'Content-Type': contentType });
  res.end(data);
});
  });
});

const port = 8080;
server.listen(port, () => {
  console.log(`Server is running on port ${port}`);
});
