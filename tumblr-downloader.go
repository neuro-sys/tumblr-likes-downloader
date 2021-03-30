//    Tumblr Downloader is an application that authenticates to your Tumblr
//    account, and downloads all the liked photos to your computer.
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
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"net"
	"net/http"
	"net/url"
	"os"
	"path"
	"regexp"
	"strconv"
	"strings"
	"time"

	"github.com/dghubble/oauth1"
	"github.com/elgs/gojq"
	"github.com/pkg/browser"
)

const limit = 50

var (
	blogIdentifier string
	consumerKey    string
	consumerSecret string
	callbackURL    string

	config oauth1.Config

	outputFolderName string
	startOffset      string
	debug            bool
	downloadNoDirs   bool

	reMediaURL = regexp.MustCompile("src=\"([^\"]+)\"")
)

var lock chan string
var httpClient *http.Client

func checkError(err error) bool {
	if err != nil {
		fmt.Println("There was an error:")
		fmt.Println(err)
		time.Sleep(500 * time.Millisecond)
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
	blogIdentifier = getConfigValue("BLOG_IDENTIFIER")
	consumerKey = getConfigValue("CONSUMER_KEY")
	consumerSecret = getConfigValue("CONSUMER_SECRET")
	callbackURL = getConfigValue("CALLBACK_URL")

	blogIdentifier = strings.Replace(blogIdentifier, "http://", "", -1)
	blogIdentifier = strings.Replace(blogIdentifier, "https://", "", -1)
	blogIdentifier = strings.Replace(blogIdentifier, ".tumblr.com", "", -1)
	blogIdentifier = blogIdentifier + ".tumblr.com"

	outputFolderName = getConfigValue("TARGET_LOCATION") + string(os.PathSeparator) + blogIdentifier
	os.Mkdir(outputFolderName, os.ModePerm)

	startOffset = getConfigValue("START_OFFSET")
	debug, _ = strconv.ParseBool(getConfigValue("DEBUG_MODE"))
	downloadNoDirs, _ = strconv.ParseBool(getConfigValue("DOWNLOAD_NO_DIRS"))
}

func authTumblr() {
	requestToken, requestSecret, _ := config.RequestToken()
	authorizationURL, _ := config.AuthorizationURL(requestToken)

	fmt.Println("Authorization URL: " + authorizationURL.String())
	fmt.Println("Please check your browser, and log into Tumblr as usual, then come back to this application.")

	browser.OpenURL(authorizationURL.String())

	listen, err := net.Listen("tcp", ":8080")
	checkError(err)

	http.HandleFunc("/tumblr/callback", func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprint(w, "Success! Now go check the app!")
		verifier := r.URL.Query().Get("oauth_verifier")
		token2 := r.URL.Query().Get("oauth_token")

		fmt.Println(verifier, token2)

		listen.Close()

		accessToken, accessSecret, err := config.AccessToken(token2, requestSecret, verifier)
		checkError(err)

		token := oauth1.NewToken(accessToken, accessSecret)

		ctx := context.Background()

		httpClient = config.Client(ctx, token)

		fmt.Println("Releasing now")
		time.Sleep(1000 * time.Millisecond)
		lock <- "Go"
	})

	http.Serve(listen, nil)
}

func tumblrGetLikes(blogIdentifier string, timestamp int) *gojq.JQ {
	urlPath := "http://api.tumblr.com/v2/blog/" + blogIdentifier + "/likes?&api_key=" + consumerKey + "&limit=" + strconv.Itoa(limit)

	if timestamp == 0 {
		urlPath += "&offset=" + startOffset
	}

	if timestamp != 0 {
		urlPath += "&before=" + strconv.Itoa(timestamp)
	}

	if debug {
		fmt.Println("[DEBUG] - " + urlPath)
	}

	resp, err := httpClient.Get(urlPath)
	checkError(err)

	body, err := ioutil.ReadAll(resp.Body)
	checkError(err)

	parser, err := gojq.NewStringQuery(string(body))
	checkError(err)

	if debug {
		mapRes, _ := parser.QueryToMap(".")
		res, _ := json.MarshalIndent(mapRes, "", "  ")
		fmt.Println("[DEBUG] - " + string(res))
	}

	return parser
}

// Check whether the given URL belongs to tumblr.com
func isTumblr(URL string) bool {
	parsedURL, err := url.Parse(URL)
	checkError(err)

	return strings.Contains(parsedURL.Hostname(), "tumblr.com")
}

func downloadURL(URL string, SubDir string) {
	fileName := ""

	if isTumblr(URL) {
		fmt.Println("Downloading", URL)
		fileName = path.Base(URL)
	}

	if fileName != "" {
		var outputFilePath string

		if downloadNoDirs {
			outputFilePath = outputFolderName + string(os.PathSeparator) + fileName
		} else {
			outputFilePath = outputFolderName + string(os.PathSeparator) + SubDir + fileName
			os.Mkdir(outputFolderName + string(os.PathSeparator)+SubDir, os.ModePerm)
		}

		_, err := os.Stat(outputFilePath)
		if os.IsNotExist(err) {
			resp, err := http.Get(URL)

			outFile, err := os.Create(outputFilePath)
			checkError(err)

			defer outFile.Close()
			_, err = io.Copy(outFile, resp.Body)
		} else {
			fmt.Println("File already exists, skipping")
		}
	} else {
		fmt.Println("No tumblr link, skipping")
	}
}

func run() {
	lastTimestamp := 0

	counter := 1
	counterStartOffset, _ := strconv.ParseInt(startOffset, 10, 32)
	counter = counter + int(counterStartOffset)
	for {
		fmt.Printf("Ask for liked posts %d - %d... ", counter, counter+limit-1)
		parser := tumblrGetLikes(blogIdentifier, lastTimestamp)

		likedPosts, err := parser.QueryToArray("response.liked_posts")
		checkError(err)
		if len(likedPosts) == 0 {
			fmt.Println("0 rows fetched")
			os.Exit(1)
		} else {
			fmt.Println("done")
		}

		for _, v := range likedPosts {
			postType, err := gojq.NewQuery(v).Query("type")
			checkError(err)
			postBlogName, err := gojq.NewQuery(v).Query("blog_name")
			checkError(err)

			lastTimestampString, err := gojq.NewQuery(v).Query("liked_timestamp")
			checkError(err)

			lastTimestamp = int(lastTimestampString.(float64))

			if postType == "photo" {
				postPhotos, err := gojq.NewQuery(v).QueryToArray("photos")
				checkError(err)

				for i, v2 := range postPhotos {
					url, err := gojq.NewQuery(v2).Query("original_size.url")
					checkError(err)

					switch url.(type) {
					case string:
						fmt.Print("[" + strconv.Itoa(counter) + "/" + strconv.Itoa(i) + "] ")
						downloadURL(url.(string), postBlogName.(string)+string(os.PathSeparator))
					}
				}
			}

			if postType == "video" {
				videoUrl, err := gojq.NewQuery(v).Query("video_url")

				if err == nil && videoUrl != nil {
					switch videoUrl.(type) {
					case string:
						fmt.Print("[" + strconv.Itoa(counter) + "] ")
						downloadURL(videoUrl.(string), postBlogName.(string)+string(os.PathSeparator))
					}
				} else {
					postPlayer, err := gojq.NewQuery(v).QueryToArray("player")
					checkError(err)

					for _, v2 := range postPlayer {
						url, err := gojq.NewQuery(v2).Query("embed_code")
						checkError(err)

						switch url.(type) {
						case string:
							matches := reMediaURL.FindAllStringSubmatch(url.(string), -1)
							for _, match := range matches {
								urlVideo := match[1]
								fmt.Println("Downloading object from multi-media bundle")
								downloadURL(urlVideo, postBlogName.(string)+string(os.PathSeparator))
							}
						}
					}
				}
			}

			if postType == "text" {
				postBody, err := gojq.NewQuery(v).Query("body")
				checkError(err)

				if debug {
					fmt.Println("Downloading media from text post")
				}
				matches := reMediaURL.FindAllStringSubmatch(postBody.(string), -1)
				for _, match := range matches {
					urlVideo := match[1]
					downloadURL(urlVideo, postBlogName.(string)+string(os.PathSeparator))
				}
			}

			counter += 1
		}
	}
}

func main() {
	fmt.Println("Initializing environment... ")
	loadConfig()
	initOauthConfig()

	lock = make(chan string, 1)
	fmt.Println("Authorizing on tumblr... ")

	go func() {
		fmt.Println("Waiting until authorization flow is completed")
		<-lock
		fmt.Println("Authorization complete")
		run()
		lock <- "Done"
	}()

	authTumblr()
	<-lock
}
