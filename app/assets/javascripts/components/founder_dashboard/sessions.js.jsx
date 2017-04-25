class FounderDashboardSessions extends React.Component {
  constructor(props) {
    super(props);

    this.state = {
      filterTags: []
    };

    this.chooseTags = this.chooseTags.bind(this);
  }

  upcomingSessions() {
    return this.filteredSessions().filter(function (target) {
      return moment(target.session_at).isAfter(moment());
    }).reverse();
  }

  pastSessions() {
    return this.filteredSessions().filter(function (target) {
      return moment(target.session_at).isBefore(moment());
    });
  }

  filteredSessions() {
    let that = this;

    return this.props.sessions.filter(function (target) {
      return that.targetHasTags(target, that.state.filterTags);
    })
  }

  targetHasTags(target, tags) {
    let targetTags = target.taggings.map(function (tagging) {
      return tagging.tag.name;
    });

    for (let tagIndex in tags) {
      if (!targetTags.includes(tags[tagIndex])) {
        return false;
      }
    }

    return true;
  }

  chooseTags(tags) {
    this.setState({filterTags: tags});
  }

  render() {
    return (
      <div>
        <FounderDashboardActionBar filter='sessions' filterData={ {tags: this.props.sessionTags} }
          openTimelineBuilderCB={ this.props.openTimelineBuilderCB } pickFilterCB={ this.chooseTags }/>
        <FounderDashboardTargetCollection key='sessions-upcoming' name='Upcoming Sessions' displayDate={ true }
          targets={ this.upcomingSessions() } openTimelineBuilderCB={ this.props.openTimelineBuilderCB }
          iconPaths={ this.props.iconPaths } founderDetails={ this.props.founderDetails}/>
        <FounderDashboardTargetCollection key='sessions-past' name='Past Sessions' displayDate={ true }
          targets={ this.pastSessions() } openTimelineBuilderCB={ this.props.openTimelineBuilderCB }
          finalCollection={ true } iconPaths={ this.props.iconPaths }
                                          founderDetails={ this.props.founderDetails}/>
      </div>
    );
  }
}

FounderDashboardSessions.propTypes = {
  sessions: React.PropTypes.array,
  sessionTags: React.PropTypes.array,
  openTimelineBuilderCB: React.PropTypes.func,
  iconPaths: React.PropTypes.array
};