# CoolBeeBox API

Node.js + Express 后端服务，负责 MySQL 历史数据查询、统计信息、服务健康检查，以及 DeepSeek AI 报告/对话接口代理。

## 运行

```bash
cp .env.example .env
npm install
npm start
```

请先填写 `.env` 中的数据库与 DeepSeek 配置；真实 `.env` 不应提交到 Git。

也可以使用 `docker-compose.yml` 启动容器。该 Compose 文件引用外部 Docker 网络 `1panel-network`，部署前请确认该网络存在，或按实际环境修改网络配置。
