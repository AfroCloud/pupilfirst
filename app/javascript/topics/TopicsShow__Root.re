[%bs.raw {|require("./TopicsShow__Root.css")|}];

open TopicsShow__Types;

let str = React.string;

type state = {
  topic: Topic.t,
  firstPost: Post.t,
  replies: array(Post.t),
  replyToPostId: option(string),
  topicTitle: string,
  savingTopic: bool,
  showTopicEditor: bool,
  topicCategory: option(TopicCategory.t),
};

type action =
  | SaveReply(Post.t, option(string))
  | AddNewReply(option(string))
  | LikeFirstPost
  | RemoveLikeFromFirstPost
  | LikeReply(Post.t)
  | RemoveLikeFromReply(Post.t)
  | UpdateFirstPost(Post.t)
  | UpdateReply(Post.t)
  | RemoveReplyToPost
  | ArchivePost(string)
  | UpdateTopicTitle(string)
  | SaveTopic(Topic.t)
  | ShowTopicEditor(bool)
  | UpdateSavingTopic(bool)
  | MarkReplyAsSolution(string)
  | UpdateTopicCategory(option(TopicCategory.t));

let reducer = (state, action) => {
  switch (action) {
  | SaveReply(newReply, replyToPostId) =>
    switch (replyToPostId) {
    | Some(id) =>
      let updatedParentPost =
        state.replies |> Post.find(id) |> Post.addReply(newReply |> Post.id);
      {
        ...state,
        replies:
          state.replies
          |> Js.Array.filter(r => Post.id(r) != id)
          |> Array.append([|newReply, updatedParentPost|]),
        replyToPostId: None,
      };
    | None => {
        ...state,
        replies: state.replies |> Array.append([|newReply|]),
      }
    }
  | AddNewReply(replyToPostId) => {...state, replyToPostId}
  | LikeFirstPost => {...state, firstPost: state.firstPost |> Post.addLike}
  | RemoveLikeFromFirstPost => {
      ...state,
      firstPost: state.firstPost |> Post.removeLike,
    }
  | LikeReply(post) =>
    let updatedPost = post |> Post.addLike;
    {
      ...state,
      replies:
        state.replies
        |> Js.Array.filter(reply => Post.id(reply) != Post.id(post))
        |> Array.append([|updatedPost|]),
    };
  | RemoveLikeFromReply(post) =>
    let updatedPost = post |> Post.removeLike;
    {
      ...state,
      replies:
        state.replies
        |> Js.Array.filter(reply => Post.id(reply) != Post.id(post))
        |> Array.append([|updatedPost|]),
    };
  | UpdateTopicTitle(topicTitle) => {...state, topicTitle}
  | UpdateFirstPost(firstPost) => {...state, firstPost}
  | UpdateReply(reply) => {
      ...state,
      replies:
        state.replies
        |> Js.Array.filter(r => Post.id(r) != Post.id(reply))
        |> Array.append([|reply|]),
    }
  | ArchivePost(postId) => {
      ...state,
      replies: state.replies |> Js.Array.filter(r => Post.id(r) != postId),
    }
  | RemoveReplyToPost => {...state, replyToPostId: None}
  | UpdateSavingTopic(savingTopic) => {...state, savingTopic}
  | SaveTopic(topic) => {
      ...state,
      topic,
      savingTopic: false,
      showTopicEditor: false,
    }
  | ShowTopicEditor(showTopicEditor) => {...state, showTopicEditor}
  | MarkReplyAsSolution(postId) => {
      ...state,
      replies: state.replies |> Post.markAsSolution(postId),
    }
  | UpdateTopicCategory(topicCategory) => {...state, topicCategory}
  };
};

let addNewReply = (send, replyToPostId, ()) => {
  send(AddNewReply(replyToPostId));
};

let updateReply = (send, reply) => {
  send(UpdateReply(reply));
};

let updateFirstPost = (send, post) => {
  send(UpdateFirstPost(post));
};

let saveReply = (send, replyToPostId, reply) => {
  send(SaveReply(reply, replyToPostId));
};

let isTopicCreator = (firstPost, currentUserId) => {
  Post.creatorId(firstPost)
  ->Belt.Option.mapWithDefault(false, id => id == currentUserId);
};

let archiveTopic = community => {
  community
  |> Community.path
  |> Webapi.Dom.Window.setLocation(Webapi.Dom.window);
};

module UpdateTopicQuery = [%graphql
  {|
  mutation UpdateTopicMutation($id: ID!, $title: String!, $topicCategoryId: ID) {
    updateTopic(id: $id, title: $title, topicCategoryId: $topicCategoryId)  {
      success
    }
  }
|}
];

let updateTopic = (state, send, event) => {
  event |> ReactEvent.Mouse.preventDefault;
  send(UpdateSavingTopic(true));
  let topicCategoryId =
    Belt.Option.flatMap(state.topicCategory, category =>
      Some(TopicCategory.id(category))
    );
  UpdateTopicQuery.make(
    ~id=state.topic |> Topic.id,
    ~title=state.topicTitle,
    ~topicCategoryId?,
    (),
  )
  |> GraphqlQuery.sendQuery
  |> Js.Promise.then_(response => {
       response##updateTopic##success
         ? {
           let topic = state.topic |> Topic.updateTitle(state.topicTitle);
           send(SaveTopic(topic));
         }
         : send(UpdateSavingTopic(false));
       Js.Promise.resolve();
     })
  |> Js.Promise.catch(_ => {
       send(UpdateSavingTopic(false));
       Js.Promise.resolve();
     })
  |> ignore;
};

let communityLink = community => {
  <a href={Community.path(community)} className="btn btn-subtle">
    <i className="fas fa-users" />
    <span className="ml-2"> {Community.name(community) |> str} </span>
  </a>;
};

let topicCategory = (topicCategories, topicCategoryId) => {
  switch (topicCategoryId) {
  | Some(id) =>
    Some(
      ArrayUtils.unsafeFind(
        category => TopicCategory.id(category) == id,
        "Unable to find topic category with ID: " ++ id,
        topicCategories,
      ),
    )
  | None => None
  };
};

let categoryDropdownSelected = topicCategory => {
  <div
    ariaLabel="Selected category"
    className="text-sm bg-gray-100 border border-gray-400 rounded py-1 px-3 mt-1 focus:outline-none focus:bg-white focus:border-primary-300 cursor-pointer">
    {switch (topicCategory) {
     | Some(topicCategory) =>
       let (color, _) = TopicCategory.color(topicCategory);
       let style = ReactDOMRe.Style.make(~backgroundColor=color, ());

       <div className="inline-flex items-center">
         <div className="h-3 w-3 border" style />
         <span className="ml-2">
           {TopicCategory.name(topicCategory)->str}
         </span>
       </div>;
     | None => str("None")
     }}
    <FaIcon classes="ml-4 fas fa-caret-down" />
  </div>;
};

let topicCategorySelector =
    (send, selectedTopicCategory, availableTopicCategories) => {
  let selectableTopicCategories =
    Belt.Option.mapWithDefault(
      selectedTopicCategory, availableTopicCategories, topicCategory => {
      Js.Array.filter(
        availableTopicCategory =>
          TopicCategory.id(availableTopicCategory)
          != TopicCategory.id(topicCategory),
        availableTopicCategories,
      )
    });

  let topicCategoryList =
    Js.Array.map(
      topicCategory => {
        let (color, _) = TopicCategory.color(topicCategory);
        let style = ReactDOMRe.Style.make(~backgroundColor=color, ());
        let categoryName = TopicCategory.name(topicCategory);

        <div
          ariaLabel={"Select category " ++ categoryName}
          className="pl-3 pr-4 py-2 font-normal flex items-center"
          onClick={_ => send(UpdateTopicCategory(Some(topicCategory)))}>
          <div className="w-4 h-4 border" style />
          <span className="ml-2"> categoryName->str </span>
        </div>;
      },
      selectableTopicCategories,
    );

  switch (selectedTopicCategory) {
  | None => topicCategoryList
  | Some(_category) =>
    Js.Array.concat(
      topicCategoryList,
      [|
        <div
          ariaLabel="Select no category"
          className="pl-3 pr-4 py-2 font-normal flex items-center"
          onClick={_ => send(UpdateTopicCategory(None))}>
          <div className="w-4 h-4" />
          <span className="ml-2"> "None"->str </span>
        </div>,
      |],
    )
  };
};

[@react.component]
let make =
    (
      ~topic,
      ~firstPost,
      ~replies,
      ~users,
      ~currentUserId,
      ~moderator,
      ~community,
      ~target,
      ~topicCategories,
    ) => {
  let (state, send) =
    React.useReducerWithMapState(reducer, topic, topic =>
      {
        topic,
        firstPost,
        replies,
        replyToPostId: None,
        topicTitle: topic |> Topic.title,
        savingTopic: false,
        showTopicEditor: false,
        topicCategory:
          topicCategory(topicCategories, Topic.topicCategoryId(topic)),
      }
    );

  <div className="bg-gray-100">
    <div className="max-w-4xl w-full mt-5 pl-4 lg:pl-0 lg:mx-auto">
      {communityLink(community)}
    </div>
    <div className="flex-col items-center justify-between">
      {switch (target) {
       | Some(target) =>
         <div className="max-w-4xl w-full mt-5 lg:x-4 mx-auto">
           <div
             className="flex py-4 px-4 md:px-5 mx-3 lg:mx-0 bg-white border border-primary-500 shadow-md rounded-lg justify-between items-center">
             <p className="w-3/5 md:w-4/5 text-sm">
               <span className="font-semibold block text-xs">
                 {"Linked Target: " |> str}
               </span>
               <span> {target |> LinkedTarget.title |> str} </span>
             </p>
             {switch (target |> LinkedTarget.id) {
              | Some(id) =>
                <a href={"/targets/" ++ id} className="btn btn-default">
                  {"View Target" |> str}
                </a>
              | None => React.null
              }}
           </div>
         </div>
       | None => React.null
       }}
      <div
        className="max-w-4xl w-full mx-auto bg-white p-4 lg:p-8 my-4 border-t border-b md:border-0 lg:rounded-lg lg:shadow">
        {<div ariaLabel="Topic Details">
           {state.showTopicEditor
              ? <DisablingCover disabled={state.savingTopic}>
                  <div
                    className="flex flex-col lg:ml-14 bg-gray-100 p-2 rounded border border-primary-200">
                    <input
                      onChange={event =>
                        send(
                          UpdateTopicTitle(
                            ReactEvent.Form.target(event)##value,
                          ),
                        )
                      }
                      value={state.topicTitle}
                      className="appearance-none block w-full bg-white text-gray-900 font-semibold border border-gray-400 rounded py-3 px-4 mb-2 leading-tight focus:outline-none focus:bg-white focus:border-gray-500"
                      type_="text"
                    />
                    <div className="flex justify-between items-end">
                      <div className="flex flex-col items-left">
                        <span
                          className="inline-block text-gray-700 text-tiny font-semibold mr-2">
                          {"Topic Category: " |> str}
                        </span>
                        <Dropdown
                          selected={categoryDropdownSelected(
                            state.topicCategory,
                          )}
                          contents={topicCategorySelector(
                            send,
                            state.topicCategory,
                            topicCategories,
                          )}
                          className=""
                        />
                      </div>
                      <div>
                        <button
                          onClick={_ => send(ShowTopicEditor(false))}
                          className="btn btn-subtle btn-small mr-2">
                          {"Cancel" |> str}
                        </button>
                        <button
                          onClick={updateTopic(state, send)}
                          disabled={state.topicTitle |> Js.String.trim == ""}
                          className="btn btn-primary btn-small">
                          {"Update Topic" |> str}
                        </button>
                      </div>
                    </div>
                  </div>
                </DisablingCover>
              : <div className="flex flex-col ">
                  <div
                    className="topics-show__title-container flex items-start justify-between mb-2">
                    <h3
                      ariaLabel="Topic Title"
                      className="leading-snug lg:pl-14 text-base lg:text-2xl w-5/6">
                      {state.topic |> Topic.title |> str}
                    </h3>
                    {moderator || isTopicCreator(firstPost, currentUserId)
                       ? <button
                           onClick={_ => send(ShowTopicEditor(true))}
                           className="topics-show__title-edit-button inline-flex items-center font-semibold p-2 md:py-1 bg-gray-100 hover:bg-gray-300 border rounded text-xs flex-shrink-0 mt-2 ml-3 lg:invisible">
                           <i className="far fa-edit" />
                           <span className="hidden md:inline-block ml-1">
                             {"Edit Topic" |> str}
                           </span>
                         </button>
                       : React.null}
                  </div>
                  {switch (state.topicCategory) {
                   | Some(topicCategory) =>
                     let (color, _) = TopicCategory.color(topicCategory);
                     let style =
                       ReactDOMRe.Style.make(~backgroundColor=color, ());
                     <div
                       className="py-2 flex items-center lg:pl-14 text-xs font-semibold">
                       <div className="w-4 h-4 border" style />
                       <span className="ml-2">
                         {TopicCategory.name(topicCategory)->str}
                       </span>
                     </div>;
                   | None => React.null
                   }}
                </div>}
           <TopicsShow__PostShow
             key={Post.id(state.firstPost)}
             post={state.firstPost}
             topic
             users
             posts={state.replies}
             currentUserId
             moderator
             isTopicCreator={isTopicCreator(firstPost, currentUserId)}
             updatePostCB={updateFirstPost(send)}
             addNewReplyCB={addNewReply(send, None)}
             addPostLikeCB={() => send(LikeFirstPost)}
             removePostLikeCB={() => send(RemoveLikeFromFirstPost)}
             markPostAsSolutionCB={() => ()}
             archivePostCB={() => archiveTopic(community)}
           />
         </div>}
        {<h5 className="pt-4 pb-2 lg:ml-14 border-b">
           {Inflector.pluralize(
              "Reply",
              ~count=Array.length(state.replies),
              ~inclusive=true,
              (),
            )
            |> str}
         </h5>}
        {state.replies
         |> Post.sort
         |> Array.map(reply =>
              <div
                key={Post.id(reply)} className="topics-show__replies-wrapper">
                <TopicsShow__PostShow
                  post=reply
                  topic
                  users
                  posts={state.replies}
                  currentUserId
                  moderator
                  isTopicCreator={isTopicCreator(firstPost, currentUserId)}
                  updatePostCB={updateReply(send)}
                  addNewReplyCB={addNewReply(send, Some(Post.id(reply)))}
                  markPostAsSolutionCB={() =>
                    send(MarkReplyAsSolution(Post.id(reply)))
                  }
                  removePostLikeCB={() => send(RemoveLikeFromReply(reply))}
                  addPostLikeCB={() => send(LikeReply(reply))}
                  archivePostCB={() => send(ArchivePost(Post.id(reply)))}
                />
              </div>
            )
         |> React.array}
      </div>
      <div className="mt-4 px-4">
        <TopicsShow__PostEditor
          id="add-reply-to-topic"
          topic
          currentUserId
          handlePostCB={saveReply(send, state.replyToPostId)}
          replyToPostId=?{state.replyToPostId}
          replies={state.replies}
          users
          removeReplyToPostCB={() => send(RemoveReplyToPost)}
        />
      </div>
    </div>
  </div>;
};
