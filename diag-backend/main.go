package main

import (
	"encoding/json"
	"fmt"
	"html/template"
	"log"
	"net/http"
	"os"
	"os/signal"
	"path"
	"strconv"
	"sync"
	"syscall"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/joho/godotenv"
	"github.com/mavihq/persian"
	ptime "github.com/yaa110/go-persian-calendar"
)

var (
	id         = ""
	username   = ""
	password   = ""
	broker     = ""
	port       = 0
	topicRX    = ""
	httpAddr   = ""
	assetsPath = ""
	lastUpdate ptime.Time
	tmpl       *template.Template

	mqttClient     mqtt.Client
	mqttClientOnce sync.Once

	wrappedList     *WrappedList
	wrappedListOnce sync.Once
)

type ECUData struct {
	Time          string  `json:"time,omitempty"`
	EngineSpeed   float64 `json:"engine,omitempty"`
	VehicleSpeed  int     `json:"vehicle,omitempty"`
	ThrottleSpeed float64 `json:"throttle,omitempty"`
}

type WrappedList struct {
	max   int
	index int
	list  []ECUData
}

func NewWrappedList(max int) *WrappedList {
	return &WrappedList{
		max:   max,
		index: -1,
		list:  []ECUData{},
	}
}

func (l *WrappedList) Add(data ECUData) {
	if len(l.list) < l.max {
		l.list = append(l.list, data)
		return
	}

	l.index++
	l.index %= l.max
	l.list[l.index] = data
}

func (l *WrappedList) Get() []ECUData {
	result := make([]ECUData, len(l.list))
	copy(result, l.list)
	return result
}

func (l *WrappedList) GetHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		w.WriteHeader(http.StatusMethodNotAllowed)
		fmt.Fprintf(w, "method %s is not supported.", r.Method)
		return
	}

	err := tmpl.Execute(w, struct {
		List       []ECUData
		LastUpdate string
	}{
		List:       l.Get(),
		LastUpdate: persian.ToPersianDigits(lastUpdate.Format("hh:mm:ss a")),
	})
	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		fmt.Fprintf(w, "unexpected error %v", err)
		return
	}
}

func OnMessage(client mqtt.Client, msg mqtt.Message) {
	// fmt.Printf("Received message: %s from topic: %s\n", msg.Payload(), msg.Topic())
	switch msg.Topic() {
	case topicRX:
		lastUpdate = ptime.New(time.Now().In(ptime.Iran()))
		var data ECUData
		err := json.Unmarshal(msg.Payload(), &data)
		if err != nil {
			log.Println(err)
		}

		data.Time = lastUpdate.Format("HH:mm:ss")

		wrappedList.Add(data)
	}

}

func OnConnect(client mqtt.Client) {
	log.Println("Connected")
}

func OnClose(client mqtt.Client, err error) {
	log.Printf("Connect lost: %v", err)
}

func main() {

	log.SetOutput(os.Stdout)
	godotenv.Load()
	id = os.Getenv("MQTT_ID")
	username = os.Getenv("MQTT_USER")
	password = os.Getenv("MQTT_PASS")
	broker = os.Getenv("MQTT_BROKER")
	topicRX = os.Getenv("MQTT_DIAG_TOPIC")
	httpAddr = os.Getenv("HTTP_ADDRESS")

	p, err := strconv.ParseInt(os.Getenv("MQTT_PORT"), 10, 64)
	if err != nil {
		log.Fatalln(err)
	}
	port = int(p)

	wd, err := os.Getwd()
	if err != nil {
		log.Fatalln(err)
	}

	assetsPath := path.Join(wd, "assets")

	tmpl = template.Must(template.ParseFiles(path.Join(assetsPath, "index.gohtml")))

	// create mqtt and http clients
	createMqttClient()
	go createHTTPServer(httpAddr)

	// topicTX := "topic/test/rx"
	token := mqttClient.Subscribe(topicRX, 1, nil)
	token.Wait()
	fmt.Printf("Subscribed to topic %s", topicRX)

	ch := make(chan os.Signal)
	signal.Notify(ch, syscall.SIGTERM, syscall.SIGINT)

	<-ch
}

func createMqttClient() {
	mqttClientOnce.Do(func() {
		mqttClient = mqtt.NewClient(createMQTTOpts(broker, port))
	})

	if token := mqttClient.Connect(); token.Wait() && token.Error() != nil {
		log.Fatalln(token.Error())
	}
}

func createHTTPServer(addr string) {
	wrappedListOnce.Do(func() {
		wrappedList = NewWrappedList(100)
	})

	http.HandleFunc("/diag", wrappedList.GetHandler)

	log.Printf("http server listening on %s...", addr)
	log.Fatalln(http.ListenAndServe(addr, nil))
}

func createMQTTOpts(broker string, port int) *mqtt.ClientOptions {
	opts := mqtt.NewClientOptions()
	opts.AddBroker(fmt.Sprintf("tcp://%s:%d", broker, port))
	opts.SetClientID(id)
	opts.SetUsername(username)
	opts.SetPassword(password)
	opts.SetDefaultPublishHandler(OnMessage)
	opts.OnConnect = OnConnect
	opts.OnConnectionLost = OnClose
	return opts
}
