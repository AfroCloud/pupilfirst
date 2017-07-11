ActiveAdmin.register BatchApplicant do
  include DisableIntercom

  menu parent: 'Admissions', label: 'Applicants'

  permit_params :name, :email, :phone, :role, :team_lead, :reference, :college_id, :notes, :fee_payment_method,
    :born_on, :gender, :parent_name, :current_address, :permanent_address, :id_proof_type, :id_proof_number, :id_proof,
    :address_proof,
    batch_application_ids: [],
    tag_list: [],
    college_attributes: %i[name also_known_as city state_id established_year website contact_numbers university_id]

  scope :all, default: true
  scope :submitted_application
  scope :payment_initiated
  scope :conversion

  filter :name
  filter :email

  filter :ransack_tagged_with,
    as: :select,
    multiple: true,
    label: 'Tags',
    collection: -> { BatchApplicant.tag_counts_on(:tags).pluck(:name).sort }

  filter :batch_applications_application_round_batch_id_eq, as: :select, collection: proc { Batch.all }, label: 'With applications in batch'

  filter :batch_applications_application_round_id_eq, as: :select, label: 'With application in round', collection: proc {
    batch_id = params.dig(:q, :batch_applications_application_round_batch_id_eq)

    if batch_id.present?
      ApplicationRound.where(batch_id: batch_id)
    else
      [['Pick batch first', '']]
    end
  }

  filter :batch_applications_application_stage_id_eq, as: :select, collection: proc { ApplicationStage.all }, label: 'With applications in stage'
  filter :fee_payment_method, as: :select, collection: -> { BatchApplicant::FEE_PAYMENT_METHODS }
  filter :phone
  filter :reference_eq, as: :select, collection: proc { BatchApplicant.reference_sources[0..-2] }, label: 'Reference (Selected)'
  filter :reference, label: 'Reference (Free-form)'
  filter :gender, as: :select, collection: proc { Founder.valid_gender_values }
  filter :college_state_id_eq, label: 'State', as: :select, collection: proc { State.all }
  filter :college_id_null, label: 'College missing?', as: :boolean
  filter :created_at

  index do
    selectable_column

    column :name
    column :phone
    column :reference

    column :college do |batch_applicant|
      if batch_applicant.college.present?
        link_to batch_applicant.college.name, admin_college_path(batch_applicant.college)
      elsif batch_applicant.college_text.present?
        span "#{batch_applicant.college_text} "
        span admin_create_college_link(batch_applicant.college_text)
      else
        content_tag :em, 'Unknown'
      end
    end

    column :state do |batch_applicant|
      if batch_applicant.college.present?
        if batch_applicant.college.state.present?
          link_to batch_applicant.college.state.name, admin_state_path(batch_applicant.college.state)
        else
          content_tag :em, 'College without state'
        end
      elsif batch_applicant.college_text.present?
        content_tag :em, 'No linked college'
      else
        content_tag :em, 'Unknown - Please fix'
      end
    end

    column :notes

    column :last_created_application do |batch_applicant|
      application = batch_applicant.batch_applications.where(team_lead_id: batch_applicant.id).last

      if application.present?
        link_to application.display_name, admin_batch_application_path(application)
      end
    end

    column :latest_payment, sortable: 'latest_payment_at' do |batch_applicant|
      payment = batch_applicant.payments.order('created_at').last
      if payment.blank?
        'No Payment'
      elsif payment.paid?
        payment.paid_at.strftime('%b %d, %Y')
      else
        "#{payment.status.capitalize} on #{payment.created_at.strftime('%b %d, %Y')}"
      end
    end

    column :tags do |batch_applicant|
      linked_tags(batch_applicant.tags, separator: ' | ')
    end

    actions
  end

  show do
    attributes_table do
      row :email
      row :name
      row :phone
      row :college
      row :college_text

      row :tags do |batch_applicant|
        linked_tags(batch_applicant.tags)
      end

      row :role

      row :applications do |batch_applicant|
        applications = batch_applicant.batch_applications

        if applications.present?
          ul do
            applications.each do |application|
              li do
                a href: admin_batch_application_path(application) do
                  application.display_name
                end

                span do
                  if application.team_lead_id == batch_applicant.id
                    ' (Team Lead)'
                  else
                    ' (Cofounder)'
                  end
                end
              end
            end
          end
        end
      end

      row :reference
      row :fee_payment_method
      row :notes
    end

    panel 'Personal Details' do
      attributes_table_for batch_applicant do
        row :born_on
        row :gender
        row :parent_name
        row :current_address
        row :permanent_address
        row :id_proof_type
        row :id_proof_number

        row :id_proof do |applicant|
          link_to 'Click here to open in new window', applicant.id_proof.url, target: '_blank' if applicant.id_proof.present?
        end

        row :address_proof do |applicant|
          link_to 'Click here to open in new window', applicant.address_proof.url, target: '_blank' if applicant.address_proof.present?
        end
      end
    end

    panel 'Technical details' do
      attributes_table_for batch_applicant do
        row :id
        row :created_at
        row :updated_at
      end
    end
  end

  csv do
    column :id
    column :name
    column :email
    column :phone
    column :gender
    column :role
    column :reference

    column :college do |batch_applicant|
      college = batch_applicant.college
      college.name if college.present?
    end

    column :college_text
    column :born_on
    column :parent_name
    column :current_address
    column :permanent_address

    column :applications do |batch_applicant|
      batch_applicant.batch_applications.map do |application|
        if application.team_lead == batch_applicant
          "Team lead on application ##{application.id}"
        else
          "Cofounder on application ##{application.id}"
        end
      end.join(', ')
    end

    column :startup do |batch_applicant|
      admitted_application = batch_applicant.batch_applications.find { |batch_application| batch_application.startup.present? }

      if admitted_application.present?
        "Startup ##{admitted_application.startup.id} - #{admitted_application.startup.product_name}"
      end
    end

    column :created_at
    column :updated_at
  end

  form do |f|
    f.semantic_errors(*f.object.errors.keys)

    f.inputs do
      f.input :batch_applications, collection: BatchApplication.all.includes(:team_lead, :application_round)
      f.input :name
      f.input :email

      f.input :tag_list,
        as: :select,
        collection: BatchApplicant.tag_counts_on(:tags).pluck(:name),
        multiple: true

      f.input :phone
      f.input :college_text, label: 'College as text'
      f.input :college_id, as: :select, input_html: { 'data-search-url' => colleges_url }, collection: f.object.college.present? ? [f.object.college] : []
      f.input :role, as: :select, collection: Founder.valid_roles
      f.input :reference
      f.input :notes
      f.input :fee_payment_method, as: :select, collection: BatchApplicant::FEE_PAYMENT_METHODS
    end

    f.inputs 'Personal details' do
      f.input :born_on, as: :datepicker
      f.input :gender, as: :select, collection: Founder.valid_gender_values
      f.input :parent_name
      f.input :current_address
      f.input :permanent_address
      f.input :id_proof_type, as: :select, collection: BatchApplicant::ID_PROOF_TYPES
      f.input :id_proof_number
      f.input :id_proof
      f.input :address_proof
    end

    unless f.object.college&.persisted?
      div class: 'aa_batch_applicant_enable_create_college' do
        span { check_box_tag 'aa_batch_applicant_enable_create_college_checkbox' }
        span { label_tag 'aa_batch_applicant_enable_create_college_checkbox', 'Enable the form to create new college?' }
      end

      f.inputs 'Create missing college', for: [:college, College.new] do |cf|
        cf.input :name
        cf.input :also_known_as
        cf.input :city
        cf.input :state_id, as: :select, collection: State.all
        cf.input :established_year
        cf.input :website
        cf.input :contact_numbers
        cf.input :university_id, as: :select, collection: University.all
      end
    end

    f.actions
  end
end
