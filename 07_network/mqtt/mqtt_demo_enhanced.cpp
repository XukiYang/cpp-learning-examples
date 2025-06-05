#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <mqtt/async_client.h> // mqtt
#include <string>
#include <thread>

// ================ 配置常量 ================
const std::string
    SERVER_ADDRESS("tcp://localhost:1883"); // Broker地址协议说明：
                                            // - tcp:// 普通TCP连接
                                            // - ssl:// TLS加密连接
                                            // - ws://  WebSocket连接
const std::string
    CLIENT_ID("cpp_demo_client"); // 客户端ID（需唯一，空字符串将自动生成）
const std::string TOPIC("test/topic"); // 主题名称规范：
                                       // - 区分大小写
                                       // - 支持层级（用/分隔）
                                       // - 避免使用#和+（保留字符）
const int QOS = 1;                     // 服务质量等级：
                                       // 0-最多一次（可能丢失）
                                       // 1-至少一次（可能重复）
                                       // 2-恰好一次（保证可靠）

// ================ 回调处理器 ================
/**
 * @class EnhancedCallback
 * @brief 增强型MQTT回调处理器，继承自mqtt::callback
 * @details 覆盖所有关键事件处理方法，包含各方法触发时机说明
 */
class EnhancedCallback : public mqtt::callback {
public:
  /**
   * @brief 消息到达回调（主业务逻辑入口）
   * @param msg 包含主题、负载、QoS等属性的常量消息指针
   * @note 该方法在独立线程中执行，需注意线程安全
   */
  void message_arrived(mqtt::const_message_ptr msg) override {
    std::cout << "[回调] 收到消息 << 主题: [" << msg->get_topic()
              << "], 负载: " << msg->to_string() << ", QoS: " << msg->get_qos()
              << std::endl;
  }

  /**
   * @brief 连接丢失回调（网络异常时触发）
   * @param cause 断开原因说明
   */
  void connection_lost(const std::string &cause) override {
    std::cerr << "[回调] 连接丢失! 原因: " << cause << std::endl;
    // 此处可添加重连逻辑（但示例中已启用自动重连）
  }

  /**
   * @brief 消息交付完成回调（QoS>0时确认到达Broker）
   * @param token 包含消息交付状态的token
   */
  void delivery_complete(mqtt::delivery_token_ptr token) override {
    std::cout << "[回调] 消息交付确认: "
              << (token->get_message() ? token->get_message()->get_topic()
                                       : "null")
              << " (消息ID:" << token->get_message_id() << ")" << std::endl;
  }
};

// ================ 主程序 ================
int main() {
  // 1. 创建异步客户端（非阻塞IO模型）
  // 参数说明：
  // - serverURI:
  // Broker地址（支持failover格式："tcp://host1:1883,tcp://host2:1883"）
  // - clientId:  客户端标识（空字符串将自动生成唯一ID）
  // - persistenceType: 持久化类型（可选，此处使用默认内存存储）
  mqtt::async_client client(SERVER_ADDRESS, CLIENT_ID);

  // 2. 设置回调处理器
  EnhancedCallback cb;
  client.set_callback(cb);

  // 3. 配置连接选项（使用Builder模式）
  mqtt::connect_options connOpts =
      mqtt::connect_options_builder()
          .clean_session(true) // 清除会话（true=新会话，false=恢复之前会话）
          .automatic_reconnect(true) // 自动重连（间隔从1秒开始指数退避）
          .max_inflight(10) // 最大未确认消息数（飞行窗口）
          .keep_alive_interval(std::chrono::seconds(60)) // 心跳间隔
          .finalize();

  // 4. 添加SSL/TLS配置示例（如需加密连接）
  /*
  auto sslOpts = mqtt::ssl_options_builder()
      .trust_store("/path/to/ca.crt")
      .key_store("/path/to/client.p12")
      .private_key("/path/to/client.key")
      .finalize();
  connOpts.set_ssl(sslOpts);
  */

  try {
    // 5. 连接Broker（异步操作转同步等待）
    std::cout << "正在连接Broker..." << std::endl;
    client.connect(connOpts)->wait(); // wait()将阻塞直到操作完成
    std::cout << "连接成功! Server: " << SERVER_ADDRESS << std::endl;

    // 6. 订阅主题（支持多主题订阅）
    // 参数说明：
    // - topicFilter: 主题过滤器（支持通配符，如"sensor/#"）
    // - qos: 最大接收质量等级
    client.subscribe(TOPIC, QOS)->wait();
    std::cout << "已订阅主题: " << TOPIC << " (QoS: " << QOS << ")"
              << std::endl;

    // 7. 创建发布线程（演示周期性发布）
    std::atomic<bool> running{true};
    std::thread publisher_thread = std::thread([&] {
      int count = 0;
      while (running) {
        // 7.1 构造消息（使用工厂方法创建智能指针）
        auto msg = mqtt::make_message(
            TOPIC,                             // 目标主题
            "消息#" + std::to_string(++count), // 负载内容
            QOS,                               // 服务质量
            false // 保留标志（true=Broker会保存最后一条消息）
        );

        // 7.2 设置消息属性（MQTT 5.0特性）
        // msg->set_correlation_data("12345"); // 关联数据
        // msg->set_response_topic("response/topic"); // 响应主题

        // 7.3 异步发布（立即返回，通过delivery_complete回调确认）
        client.publish(msg);

        std::cout << "[主线程] 已发布: " << msg->to_string()
                  << " (QoS:" << msg->get_qos() << ")" << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    });

    // 8. 主线程等待用户输入退出
    std::cout << "\n按Enter键退出程序..." << std::endl;
    std::cin.get();
    running = false;

    // 9. 清理线程
    publisher_thread.join();

    // 10. 断开连接（优雅关闭）
    std::cout << "正在断开连接..." << std::endl;
    client.disconnect()->wait();
    std::cout << "已断开连接" << std::endl;
  } catch (const mqtt::exception &exc) {
    // 错误处理（网络问题、协议错误等）
    std::cerr << "MQTT异常: " << exc.what()
              << " [错误码: " << exc.get_reason_code() << "]" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

// g++ -std=c++11 mqtt_demo_enhanced.cpp -o demo     -lpaho-mqttpp3 -lpaho-mqtt3a -lpthread -lssl -lcrypto