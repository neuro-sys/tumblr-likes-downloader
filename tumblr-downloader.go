package main

import (
	"errors"
	"fmt"
	"golang.org/x/net/context"
	"io"
	"io/ioutil"
	"net/http"
	"os"
	"regexp"
	"strconv"
	"strings"
	"time"

	"github.com/dghubble/oauth1"
	"github.com/elgs/gojq"
	"github.com/go-ini/ini"
	"github.com/headzoo/surf"
)

var (
	username       string
	password       string
	blogIdentifier string
	consumerKey    string
	consumerSecret string
	callbackURL    string

	config oauth1.Config
)

func checkError(err error) {
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
}

func initOauthConfig() {
	config = oauth1.Config{
		ConsumerKey:    consumerKey,
		ConsumerSecret: consumerSecret,
		CallbackURL:    callbackURL,
		Endpoint: oauth1.Endpoint{
			RequestTokenURL: "https://www.tumblr.com/oauth/request_token",
			AuthorizeURL:    "https://www.tumblr.com/oauth/authorize",
			AccessTokenURL:  "https://www.tumblr.com/oauth/access_token",
		},
	}
}

func getConfigValue(cfg *ini.File, key string) string {
	value, err := cfg.Section("General").GetKey(key)
	checkError(err)
	if value.String() == "" {
		checkError(errors.New(key + " is missing"))
	}
	return value.String()
}

func loadConfig() {
	cfg, err := ini.Load("tumblr-downloader.config")
	checkError(err)

	username = getConfigValue(cfg, "USERNAME")
	password = getConfigValue(cfg, "PASSWORD")
	blogIdentifier = getConfigValue(cfg, "BLOG_IDENTIFIER")
	consumerKey = getConfigValue(cfg, "CONSUMER_KEY")
	consumerSecret = getConfigValue(cfg, "CONSUMER_SECRET")
	callbackURL = getConfigValue(cfg, "CALLBACK_URL")
}

func authTumblr() *http.Client {
	requestToken, requestSecret, _ := config.RequestToken()
	authorizationURL, _ := config.AuthorizationURL(requestToken)

	bow := surf.NewBrowser()
	_ = bow.Open(authorizationURL.String())

	form, err := bow.Form("#signup_form")
	checkError(err)

	form.Input("user[email]", username)
	form.Input("user[password]", password)

	form.Submit()

	form, err = bow.Form("form")
	checkError(err)

	magicError := form.Click("allow")
	if magicError == nil {
		checkError(errors.New("There must have been an error! Do you somehow a webserver running on your computer? Turn it off!"))
	}

	re, err := regexp.Compile("oauth_verifier=(.*?)#")
	checkError(err)

	verifierMatch := re.FindAllStringSubmatch(magicError.Error(), -1)

	verifier := verifierMatch[0][1]

	accessToken, accessSecret, err := config.AccessToken(requestToken, requestSecret, verifier)
	checkError(err)

	token := oauth1.NewToken(accessToken, accessSecret)

	ctx := context.Background()

	return config.Client(ctx, token)
}

func tumblrGetLikes(httpClient *http.Client, blogIdentifier string, timestamp int) *gojq.JQ {
	path := "http://api.tumblr.com/v2/blog/" + blogIdentifier + "/likes?&api_key=" + consumerKey + "&limit=20"

	if timestamp != 0 {
		path += "&before=" + strconv.Itoa(timestamp)
	}

	resp, err := httpClient.Get(path)
	checkError(err)

	body, err := ioutil.ReadAll(resp.Body)
	checkError(err)

	parser, err := gojq.NewStringQuery(string(body))
	checkError(err)

	return parser
}

func downloadURL(URL string) {
	resp, err := http.Get(URL)
	checkError(err)

	tokens := strings.Split(URL, "/")
	fileName := tokens[len(tokens)-1]

	folderName := "downloaded_" + time.Now().String()
	os.Mkdir(folderName, os.ModePerm)

	outFile, err := os.Create(folderName + "/" + fileName)
	checkError(err)

	defer outFile.Close()
	_, err = io.Copy(outFile, resp.Body)
}

func main() {

	loadConfig()
	initOauthConfig()

	httpClient := authTumblr()
	lastTimestamp := 0

	counter := 1
	for {
		parser := tumblrGetLikes(httpClient, blogIdentifier, lastTimestamp)

		likedPosts, err := parser.QueryToArray("response.liked_posts")
		checkError(err)

		for _, v := range likedPosts {
			postType, err := gojq.NewQuery(v).Query("type")
			checkError(err)

			if postType != "photo" {
				continue
			}

			url, err := gojq.NewQuery(v).Query("photos.[0].original_size.url")
			checkError(err)

			lastTimestampString, err := gojq.NewQuery(v).Query("liked_timestamp")
			checkError(err)

			lastTimestamp = int(lastTimestampString.(float64))

			fmt.Println(url)

			downloadURL(url.(string))

			counter += 1
		}
	}
}
