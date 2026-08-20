// AI分析诊断功能 - 需要添加到 server.js 中

// 添加依赖：npm install axios
const axios = require('axios');

// AI对话历史记录（内存存储，生产环境建议使用Redis）
const chatHistories = new Map();

// 获取最近24小时的数据样本（每小时1条）
async function getRecentDataForAnalysis(pool) {
  try {
    const [rows] = await pool.execute(`
      SELECT 
        recorded_at,
        temperature,
        humidity,
        pressure,
        gas,
        uptime
      FROM bee_sensor_data
      WHERE recorded_at >= DATE_SUB(NOW(), INTERVAL 24 HOUR)
      ORDER BY recorded_at ASC
    `);
    
    // 如果数据太多，只取最近的100条
    const sampleData = rows.length > 100 ? rows.slice(-100) : rows;
    
    // 格式化为更易读的格式
    return sampleData.map(row => ({
      时间: row.recorded_at,
      温度: `${parseFloat(row.temperature).toFixed(1)}°C`,
      湿度: `${parseFloat(row.humidity).toFixed(1)}%`,
      气压: `${parseFloat(row.pressure).toFixed(1)}hPa`,
      气体阻力: `${parseFloat(row.gas).toFixed(0)}Ω`
    }));
  } catch (error) {
    console.error('获取分析数据失败:', error);
    throw error;
  }
}

// 生成今日蜂箱报告
app.post('/api/ai/report', async (req, res) => {
  try {
    // 检查API密钥
    if (!process.env.DEEPSEEK_API_KEY) {
      return res.status(500).json({
        success: false,
        error: '未配置DeepSeek API密钥'
      });
    }

    // 获取最近24小时的数据
    const data = await getRecentDataForAnalysis(pool);
    
    if (data.length === 0) {
      return res.status(404).json({
        success: false,
        error: '没有足够的数据用于分析'
      });
    }

    // 构建提示词
    const prompt = `你是一位经验丰富的养蜂专家和蜂箱数据分析师。请根据以下蜂箱传感器数据（最近24小时），生成一份详细的蜂箱健康报告。

数据样本（共${data.length}条）：
${JSON.stringify(data, null, 2)}

请生成以下内容的报告：
1. **蜂箱整体状态评估** - 基于温湿度数据评估蜂箱当前状态
2. **温湿度分析** - 分析温湿度变化趋势和异常情况
3. **环境健康度评分** - 0-100分的评分及说明
4. **问题诊断** - 如果有异常，描述问题和建议
5. **养护建议** - 基于数据分析给出专业养蜂建议
6. **预警信息** - 如果有任何需要立即关注的异常

报告要求：
- 语言简洁专业
- 使用中文
- 温度单位：°C
- 湿度单位：%
- 突出重要数据和异常
- 提供可操作的建议`;

    // 调用DeepSeek API
    const response = await axios.post(
      'https://api.deepseek.com/chat/completions',
      {
        model: 'deepseek-chat',
        messages: [
          {
            role: 'system',
            content: '你是一位专业的蜂箱数据分析师和养蜂专家，擅长分析传感器数据，提供养殖建议。'
          },
          {
            role: 'user',
            content: prompt
          }
        ],
        temperature: 0.7,
        max_tokens: 2000
      },
      {
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${process.env.DEEPSEEK_API_KEY}`
        }
      }
    );

    const report = response.data.choices[0].message.content;

    res.json({
      success: true,
      data: {
        report,
        dataCount: data.length,
        timeRange: {
          start: data[0]?.时间,
          end: data[data.length - 1]?.时间
        }
      }
    });

  } catch (error) {
    console.error('生成报告失败:', error);
    res.status(500).json({
      success: false,
      error: error.response?.data?.error?.message || '生成报告失败'
    });
  }
});

// AI对话接口
app.post('/api/ai/chat', async (req, res) => {
  try {
    const { message, deviceId = 'default' } = req.body;

    if (!message) {
      return res.status(400).json({
        success: false,
        error: '消息内容不能为空'
      });
    }

    // 检查API密钥
    if (!process.env.DEEPSEEK_API_KEY) {
      return res.status(500).json({
        success: false,
        error: '未配置DeepSeek API密钥'
      });
    }

    // 获取或创建对话历史
    if (!chatHistories.has(deviceId)) {
      chatHistories.set(deviceId, []);
    }
    const history = chatHistories.get(deviceId);

    // 添加用户消息到历史
    history.push({
      role: 'user',
      content: message
    });

    // 限制历史长度（最多保留20条）
    if (history.length > 20) {
      history.shift();
    }

    // 调用DeepSeek API
    const response = await axios.post(
      'https://api.deepseek.com/chat/completions',
      {
        model: 'deepseek-chat',
        messages: [
          {
            role: 'system',
            content: `你是CoolBeeBox蜂箱监控系统的AI助手，名为"蜂博士"。你的专长是：
1. 养蜂知识和蜂箱管理
2. 蜂箱传感器数据分析
3. 蜜蜂健康监测和疾病预防
4. 蜂箱环境优化建议
5. 蜂蜜养殖最佳实践

请用专业且友好的语言回答用户的问题。如果用户询问关于蜂箱数据或监控相关的问题，可以结合蜂箱监控系统的功能来回答。`
          },
          ...history
        ],
        temperature: 0.8,
        max_tokens: 1500
      },
      {
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${process.env.DEEPSEEK_API_KEY}`
        }
      }
    );

    const aiMessage = response.data.choices[0].message.content;

    // 添加AI回复到历史
    history.push({
      role: 'assistant',
      content: aiMessage
    });

    res.json({
      success: true,
      data: {
        message: aiMessage,
        timestamp: new Date().toISOString()
      }
    });

  } catch (error) {
    console.error('AI对话失败:', error);
    res.status(500).json({
      success: false,
      error: error.response?.data?.error?.message || 'AI响应失败'
    });
  }
});

// 清空对话历史
app.delete('/api/ai/chat/:deviceId', (req, res) => {
  const { deviceId } = req.params;
  chatHistories.delete(deviceId);
  res.json({
    success: true,
    message: '对话历史已清空'
  });
});
