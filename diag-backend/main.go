package main

import (
	"fmt"
	"os"
	"os/signal"
	"syscall"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/joho/godotenv"
)

var (
	id       = ""
	username = ""
	password = ""
)

func OnMessage(client mqtt.Client, msg mqtt.Message) {
	fmt.Printf("Received message: %s from topic: %s\n", msg.Payload(), msg.Topic())
}

func OnConnect(client mqtt.Client) {
	fmt.Println("Connected")
}

func OnClose(client mqtt.Client, err error) {
	fmt.Printf("Connect lost: %v", err)
}

func main() {

	godotenv.Load()
	id = os.Getenv("MQTT_ID")
	username = os.Getenv("MQTT_USER")
	password = os.Getenv("MQTT_PASS")

	broker := "itsloop.dev"
	port := 8883

	opts := mqtt.NewClientOptions()
	opts.AddBroker(fmt.Sprintf("tcp://%s:%d", broker, port))
	opts.SetClientID(id)
	opts.SetUsername(username)
	opts.SetPassword(password)
	opts.SetDefaultPublishHandler(OnMessage)
	opts.OnConnect = OnConnect
	opts.OnConnectionLost = OnClose
	client := mqtt.NewClient(opts)
	if token := client.Connect(); token.Wait() && token.Error() != nil {
		panic(token.Error())
	}

	topicRX := "topic/test/rx"
	topicTX := "topic/test/tx"

	// for i := 0; i < 10; i++ {
	// 	token := client.Publish(topic, 0, false, "Damn Son")
	// 	token.Wait()
	// 	fmt.Println("Published")

	// }

	token := client.Subscribe(topicRX, 1, nil)
	token.Wait()
	fmt.Printf("Subscribed to topic %s", topicRX)

	// token = client.Subscribe(topicTX, 1, nil)
	// token.Wait()
	// fmt.Printf("Subscribed to topic %s", topicTX)

	go func() {
		timer := time.Tick(time.Second)
		for range timer {
			token := client.Publish(topicTX, 1, false, "Hi from server")
			token.Wait()
			fmt.Println("published to ", topicTX)
		}
	}()

	ch := make(chan os.Signal)
	signal.Notify(ch, syscall.SIGTERM, syscall.SIGINT)

	<-ch
}
