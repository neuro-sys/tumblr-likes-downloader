//    Tumblr Downloader is an application that authenticates to your Tumblr
//    account, and downloads all the liked photos to your computer.
//    Copyright (C) 2017  Firat Salgur
//
//    This file is part of Tumblr Downloader
//
//    Tumblr Downloader is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Lesser General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Tumblr Downloader is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public License
//    along with Tumblr Downloader.  If not, see <http://www.gnu.org/licenses/>.

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

	"github.com/dghubble/oauth1"
	"github.com/elgs/gojq"
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

	outputFolderName string
)

func checkError(err error) bool {
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}

	return err != nil
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

func getConfigValue(key string) string {
	value := os.Getenv(key)

	if value == "" {
		checkError(errors.New(key + " is missing"))
	}
	return value
}

func loadConfig() {
	username = getConfigValue("USERNAME")
	password = getConfigValue("PASSWORD")
	blogIdentifier = getConfigValue("BLOG_IDENTIFIER")
	consumerKey = getConfigValue("CONSUMER_KEY")
	consumerSecret = getConfigValue("CONSUMER_SECRET")
	callbackURL = getConfigValue("CALLBACK_URL")

	outputFolderName = getConfigValue("TARGET_LOCATION") + string(os.PathSeparator) + blogIdentifier
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

	os.Mkdir(outputFolderName, os.ModePerm)

	outputFilePath := outputFolderName + string(os.PathSeparator) + fileName

	if _, err = os.Stat(outputFilePath); os.IsNotExist(err) {
		outFile, err := os.Create(outputFilePath)
		checkError(err)

		defer outFile.Close()
		_, err = io.Copy(outFile, resp.Body)
	} else {
		fmt.Println("File already exists, skipping.")
	}
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

			lastTimestampString, err := gojq.NewQuery(v).Query("liked_timestamp")
			checkError(err)

			lastTimestamp = int(lastTimestampString.(float64))

			if postType != "photo" {
				continue
			}

			postPhotos, err := gojq.NewQuery(v).QueryToArray("photos")
			checkError(err)

			for _, v2 := range postPhotos {
				url, err := gojq.NewQuery(v2).Query("original_size.url")
				checkError(err)

				fmt.Println(url)
				downloadURL(url.(string))
			}

			counter += 1
		}
	}
}
