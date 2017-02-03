package main

import (
	"errors"
	"fmt"
	"golang.org/x/net/context"
	"io/ioutil"
	"net/http"
	"os"
	"regexp"
	"strconv"

	"github.com/dghubble/oauth1"
	"github.com/elgs/gojq"
	"github.com/headzoo/surf"
)

const username = ""
const password = ""

const blogIdentifier = ""

const consumerKey = ""
const consumerSecret = ""
const callbackURL = "http://localhost/tumblr/callback"

var config = oauth1.Config{
	ConsumerKey:    consumerKey,
	ConsumerSecret: consumerSecret,
	CallbackURL:    callbackURL,
	Endpoint: oauth1.Endpoint{
		RequestTokenURL: "https://www.tumblr.com/oauth/request_token",
		AuthorizeURL:    "https://www.tumblr.com/oauth/authorize",
		AccessTokenURL:  "https://www.tumblr.com/oauth/access_token",
	},
}

func checkError(err error) {
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
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

func main() {
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

			fmt.Println(counter, url)

			counter += 1
		}
	}
}
