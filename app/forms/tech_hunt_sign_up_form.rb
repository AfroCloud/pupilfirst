class TechHuntSignUpForm < Reform::Form
  property :name, validates: { presence: true, length: { maximum: 250 } }
  property :email, virtual: true, validates: { presence: true, length: { maximum: 250 }, email: true }
  property :showcase_link,  validates: { url: true, allow_blank: true }

  def save
    Player.transaction do
      user = User.with_email(email)
      user = User.create!(email: email) if user.blank?
      Player.create!(user: user, name: name, showcase_link: showcase_link)
    end
  end
end